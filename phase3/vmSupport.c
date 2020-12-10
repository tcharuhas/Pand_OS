/******************************VMSUPPORT.C********************************************
*   
*   VmSupport hosts exception handling functions to support the operating
*   system regarding virtual memory management.  In phase 3 we only handle
*   page faults and tlb-refill events, and everything else is killed via a
*   program trap.  Helper functions such as clearSwap and rewriteTLB optimize
*   our implentation a tad bit.  A swap pool table is initialized and nulled out
*   for later use by our Pager.
*
*
*    
*   Written by: Tyler Charuhas and Alex Delaet
*
*/



#include "../h/const.h"
#include "../h/types.h"
#include "../h/initial.h"
#include "../h/initProc.h"
#include "/usr/lib/umps3/src/support/libumps/libumps.h"

/* Local Function & Semaphore Declarations */
HIDDEN int chooseVictim();
HIDDEN int flashCommand(int opType, int victimLocation, int poolID,int flashDevNum);
HIDDEN void clearSwap(int asid);
HIDDEN void rewriteTLB(int victimNum);
extern void kill(int* process);

HIDDEN int swapSem;
HIDDEN swapPool_t swapPool[SWAPSIZE];


/* 
    initTLB initializes the swap pool table of size
    SWAPSIZE (UMAXPROC*2).  All the ASIDs are set to
    -1 so as to notify our pager that it is an empty
    entry.  Since mutual exclusion is required when
    accessing the swapPool, swapSem is set to 1.

*/
void initTLB() {
    /*init swappool */
    swapSem = 1;
    int i;
    for(i = 0; i < (SWAPSIZE); i++) {
        swapPool[i].sw_asid = EMPTY;
    }

}

/* 
    toggleInts is a simple helper function
    to update the status register depending
    on the state passed.  OFF turns the
    interrupts off, and ON renables the interrupts


*/
void toggleInts(int status) {
    unsigned int currStatus = getSTATUS();
    if(status == OFF) {
        setSTATUS(currStatus & CBIT_IE_ON);
    } else {
        setSTATUS(currStatus | IE_BIT_OFF);
    }
}

/*
    ProgramTrap is used for when the cause is not a TLBL,TLBS, or TLB-refill
    event.  kill(NULL) is called which leads to a SYS2 (terminate) on the current process.
*/
void programTrap() {
    kill(NULL);
}


/* 
    elPager is the handler for TLB management errors at the user level.  
    Specifically, it only really deals with page faults.  If the cause is
    not due to a TLBL (TLB-invalid load) or TLBS (TLB-invalid store) we proceed
    and kill the user process.  Atomic code blocks (critical regions) are implemented
    when we touch the swap pool and/or read/write to pages so as to avoid any race conditions.

*/

void elPager() {

    /* pull in the current support structure, calculate the cause, and pull in the asid */
    support_t* currSupport = (support_t*) SYSCALL(GETSUPPORT, 0, 0, 0);
    int cause = (currSupport->sup_exceptState[PGFAULTEXCEPT].s_cause & FETCH_EXCCODE) >> SHIFT_CAUSE;
    int id = currSupport->sup_asid;


    /* if the cause is not because of an invalid load or store, kill */
    if(cause != TLBINVLOAD && cause != TLBINVSTORE) {
        programTrap();
    }

    /* calculate missing page number from entryHI */
    int missingPgNum = ((currSupport->sup_exceptState[PGFAULTEXCEPT].s_entryHI) & GETVPN) >> VPN_SHIFT;

    /* get mutual exclusion */
    SYSCALL(PASSEREN, (int) &swapSem , 0, 0);

    int victimNum = chooseVictim();

    /* calulate the address.  with the given info, calculating the successive
    spot is trivial */
    unsigned int victimAddr = FRME_POOL_STRT + (victimNum * PAGESIZE);

    /* position in swap pool table, status of flashCommand */
    int poolID;
    int flashStatus;

    /*check if occupied */
    if(swapPool[victimNum].sw_asid != EMPTY) {

        /* turn V bit off and rewrite TLB in an atomic code block*/
        toggleInts(OFF);
        swapPool[victimNum].sw_pageEntry->pe_entryLO = swapPool[victimNum].sw_pageEntry->pe_entryLO & V_BIT_OFF;
        rewriteTLB(victimNum);
        toggleInts(ON);
        

        /* calculate position in pool, pull in asid from pool entry */
        poolID = (swapPool[victimNum].sw_pageNum) % MAXPGNUM;
        int victimID = swapPool[victimNum].sw_asid;

        /* write over the occupied entry in the backing store */
        flashStatus = flashCommand(WRITEFLASH, victimAddr, poolID, (victimID-1));

        if(flashStatus != READY) {
            kill(&swapSem);
        }
    }

    /* calcuate position in pool, read the entry in from the backing store */
    poolID = missingPgNum % MAXPGNUM;
    flashStatus = flashCommand(READFLASH, victimAddr, poolID, (id-1));

    if(flashStatus != READY) {
        kill(&swapSem);
    }

    /* update swap table */
    pageEntry_t* pgEntry = &(currSupport->pageTable[poolID]);
    swapPool[victimNum].sw_asid = id;
    swapPool[victimNum].sw_pageNum = missingPgNum;
    swapPool[victimNum].sw_pageEntry = pgEntry;

    /* set the entryLO with the victimAddr and turn Valid/Dirty bits on in an atomic code block */
    toggleInts(OFF);
    swapPool[victimNum].sw_pageEntry->pe_entryLO = victimAddr | V_BIT_ON | D_BIT_ON;
    rewriteTLB(victimNum);
    toggleInts(ON);

    /* release mutual exclusion, load the state and try again...*/
    SYSCALL (VERHOGEN, (int) &swapSem, 0, 0);
    LDST(&(currSupport->sup_exceptState[PGFAULTEXCEPT]));

}

/* 
    chooseVictim is a basic page replacement algorithm
    which chooses frame n+1 where n is the previously
    chosen page number.  An iteration trough the table
    is performed first to check if any frame is empty (-1 ASID)

    Benefits: we save ourselves from a second I/O in the pager.
    Drawbacks: goes from O(1) to O(n), but this is worth the tradeoff 


*/
int chooseVictim() {
    /* iterate through to check for empty swap entry (ASID == -1) */
    int i;
    static int frame = 0;

    for(i=0; i <= SWAPSIZE; i++) {
        if(swapPool[i].sw_asid == EMPTY) {
            frame = i;
            return frame;
        }
    }
    /* give us the next page table entry number */
    frame = (frame+1)%SWAPSIZE;
    return frame;
}

/*
    flashCommand is responsible for flash I/O operations 
    (READFLASH, page faults, and WRITEFLASH).  One needs to
    ensure that a P operation is performed prior to this function,
    as it assumes mutual exclusion to accessing device registers.
    An atomic state is entered upon issuing a command to the device.


*/
int flashCommand(int command, int victimLocation, int poolID, int flashDevNum) {

    /* 
       command -> request type (READ or WRITE) 
       flashDevNum -> flash device to be operated on
       poolID -> the location for the I/O in the swap pool
       victimLocation is the 4k buffer 
    */

    devregarea_t* devRegs = (devregarea_t*) RAMBASEADDR;
    toggleInts(OFF);
    /* fill d_command with the command type, fill d_data0 with victimLocation*/
    devRegs->devreg[flashDevNum+8].d_data0 = victimLocation;
    devRegs->devreg[flashDevNum+8].d_command = (poolID << 8) | command;


    int commandState = SYSCALL(IOWAIT, FLASHINT, flashDevNum, 0);
    toggleInts(ON);

    /* If something went wrong, return the negative */
    if(commandState != READY) {
        commandState = 0 - commandState;
    }

    return commandState;
}

/*
    kill is responsible for gracefully terminating a process, and
    mutual exlusion is released if the semaphore passed has it.
    To make this process graceful, the mainSemaphore is notified
    before finally calling a Sys 2 on the current proc.

*/
void kill(int* sem) {

    /* clear swap entry(s) in the swap pool table*/
    clearSwap(currentproc->p_supportStruct->sup_asid);
    /* if mutual exclusion was granted, remove it */
    if(sem != NULL) {
        SYSCALL(VERHOGEN,*sem,0,0);
    }

    /* notify the main sem so we can move on */
    SYSCALL(VERHOGEN, (int) &mainSem,0,0);

    /* finally, continue with the murder */
    SYSCALL(KILLPROC,0,0,0);
}

/*
    We wake up in the tlb refill handler whenever a TLB exception
    occurs in the nucleus.  Whatever is stored in the correct page entry
    is loaded in randomly into the TLB.

*/
void uTLB_RefillHandler() {

    /* pull in the state */
    state_t* systemState = (state_t*) BIOSDATAPAGE;


    /* pull the page num from entryHI and mod it with MXPGNUM to get a number between 0-MXPGNUM */
    int pgNum = (((systemState->s_entryHI) & GETVPN) >> VPN_SHIFT) % MAXPGNUM;

    /* load pageEntry */
    setENTRYHI(currentproc->p_supportStruct->pageTable[pgNum].pe_entryHI);
    setENTRYLO(currentproc->p_supportStruct->pageTable[pgNum].pe_entryLO);

    /* write to random location*/
    TLBWR();
 
    LDST(systemState);
}

/* 
    clearSwap is called right before killing a process.  
    This function takes the asid of the process being killed,
    and iterates through the swapPool to set the ASID to -1.
    This is a helper function for our page replacement algorithm.

*/
void clearSwap(int asid) {
    int i;
    for(i=0; i <= SWAPSIZE; i++) {
        if(swapPool[i].sw_asid == asid) {
            swapPool[i].sw_asid = EMPTY;
        }
    }
}

/*
    Update the entry in the TLB if the entry correlating
    to entryHI is already present.  If it is not, a TLBCLR()
    is issued so we can move on.
*/
void rewriteTLB(int victimNum) {

    /* update the entry */
    setENTRYHI(swapPool[victimNum].sw_pageEntry->pe_entryHI);
    TLBP();
    unsigned int present = getINDEX();

    /* shift over the index register and check */
    if(((present) >> (PRESENT_SHIFT)) != OFF){
        /* freak out and kill it all*/
        TLBCLR();
    }
    else{
        /* successfully update it*/
        setENTRYLO(swapPool[victimNum].sw_pageEntry->pe_entryLO);
        setENTRYHI(swapPool[victimNum].sw_pageEntry->pe_entryHI);
        TLBWI();
    }

}