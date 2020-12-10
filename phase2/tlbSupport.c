#include "../h/const.h"
#include "../h/types.h"
#include "/usr/local/include/umps3/umps/libumps.h"

HIDDEN semd_t* swapSem;
HIDDEN swapPool_t* swapPool[UPROCMAX*2];

void initTLB() {
    /*init swappool */
    swapSem = 1;
    for(int i = 0; i < (UPROCMAX*2); i++) {
        swapPool[i]->sw_asid = -1;
    }

}
void toggleInts(int status) {
    if(status == OFF) {
        setSTATUS(ALLOFF & CBIT_IE_ON);
    } else {
        setSTATUS(ALLOFF | CBIT_IE_ON);
    }
}


void uTLB_RefillHandler() {
    state_t* systemState = (state_t*) BIOSDATAPAGE;
    unsigned int pgNum = (((systemState.s_entryHI) & GETPAGENUM) >> VPN_SHIFT) % MAXPGNUM;

    pageEntry_t* pageEntry = currentproc->p_supportStruct.pageTable[pgNum];

    /* load pageEntry */
    setENTRYHI(pageEntry->entryHI);
    setENTRYLO(pageEntry->entryLO);
    TLBWR();

    LDST(systemState);
}

void elPager() {
    support_t* currSupport = SYSCALL (GETSUPPORT, 0, 0, 0);
    state_t* saveState = currSupport->sup_exceptionState[GENERALEXCEPT];
    int cause = (saveState->s_cause & FETCH_EXCCODE) >> SHIFT_CAUSE;
    int id = currSupport->sup_asid;

    if(cause != TLBINVL && cause != TLBINVS) {
        kill(NULL);
    }


    SYSCALL (PASSEREN, &swapSem , 0, 0);

    unsigned int missingPgNum = (saveState->s_entryHI & GETPAGENUM) >> VPN_SHIFT;
    int victimNum = chooseVictim();
    pageEntry_t* victim = swapPool[victimNum];
    int victimAddr = FRAMESTART + (victimNum * PAGESIZE);
    int storeID;
    int flashStatus;

    /*check if occupied */
    if(victim->sw_asid != -1) {
        toggleInts(OFF);
        /*turn V bit off*/
        victim->sw_pageEntry->entryLO = victim->sw_pageEntry->entryLO & 0xFFFFFDFF;
        TLBCLR();
        toggleInts(ON);
        
        /*update backing store...*/
        storeID = (swapPool[victimNum].sw_pageNum) % MAXPGNUM;
        flashStatus = flashCommand((swapPool[victimNum]->sw_asid, storeID, victimAddr, WRITEFLASH));

        if(flashStatus != READY) {
            kill(&swapSem);
        }
    }

        /* read pg table from backing store, update V-bit/VPN, TLBCLR() */
    storeID = missingPgNum % MAXPGNUM;
    flashStatus = flashCommand((id-1), storeID, victimAddr, READFLASH);
    if(flashStatus != READY) {
        kill()
    }

    toggleInts(OFF);
    swapPool[victimNum].sw_pageEntry->entryLO = victimAddr | V_BIT_ON | D_BIT_ON;
    TLBCLR();
    toggleInts(ON)

    SYSCALL (VERHOGEN, &swapSem, 0, 0);
    LDST(saveState);

}

int chooseVictim() {
     static int i = 0;
     i = (i+1)%SWAPSIZE;
     return i;
}

int flashCommand(int flashDevNum, int storeID, int victimLocation, int opType) {

    /* flashDevNum is the flash dev num, storeID is the specific I/o section num, victimLocation is the 4k buffer */

    devregarea_t* devRegs = (devregarea_t*) RAMFLOORADDR;

    toggleInts(OFF);
    /* fill the command with the op type, fill the data0 with 4k buffer location */
    devRegs->devreg[flashDevNum+8].d_command = (storeID << 8) | opType;
    devRegs->devreg[flashDevNum+8].d_data0 = victimLocation;
    int opStatus = SYSCALL(IOWAIT, INTERRUPT_FLASH, flashDevNum, 0);
    toggleInts(OFF);

    if(opStatus != READY) {
        return -1;
    }

    return opStatus;
}

void kill()