/*************************initProc.c*********************************
 * 
 * initProc is the file that contains our kernal mode process, test,
 * which initializes all of the User process we will be running.
 * Also inclued in this file is a helper fuction to create user procs
 * and the global variables such as the device semaphores, the support_t
 * structs and the mainSem that keeps track of the number of proccess 
 * running before allowing Test to terminate.
 * 
 * 
 * 
 * Written by: Tyler Charuhas and Alex Delaet
 * 
 */



#include "../h/const.h"
#include "../h/types.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
int mainSem;
int devSem[NUMDEVS];
HIDDEN void createUProc(int id);
static support_t supStructs[UPROCMAX+1];


/* 
 * 
 * Test is the main proccess that is run by the nucleus
 * and creates all u-procs. It initializes the semaphores
 * and calls the set up for the swap pool (initTLB).
 * After the setup of proccess, Test will go to sleep and
 * after the user proccess are completed it will wake up
 * and terminate.
 * 
 */
void test() {
	/* initialize the dev sems to 1 */
	int i;
	for(i=0; i < (NUMDEVS-1); i++) {
		devSem[i] = 1;
	}	

    /* calls initTLB from vmSupport to set up swap semaphores */
	initTLB();

	/* create the Uprocs */	
	int id;
	for(id=1; id <= UPROCMAX; id++) {
		createUProc(id);
	}
    
    /*initialize the main semaphore and then for each proccess
     * calls sys 3 to make test sleep until all Uprocs are completed */
	mainSem = 0;
	for(i=0; i < UPROCMAX; i++) {
		SYSCALL(PASSEREN, (int) &mainSem, 0, 0);
	}
    
    /*See you space cowboy*/
	SYSCALL(KILLPROC, 0, 0, 0);

} 


/* 
 * 
 * This the create user helper function that take the ASID that the process will be using. 
 * This function will set up the context support structs and the state of the proccess 
 * and initialize the page table for that proccess as well. Once the proccess has been
 * set up this fuction will call sys1 to create a new proccess will the state and support structs
 * that had been set up.
 * 
 */
void createUProc(int id) {
    /*variables to get the memory locations to set up the context stack pointers*/
    memaddr ramTOP;
    RAMTOP(ramTOP);
    memaddr topStack = ramTOP - (2*id*PAGESIZE);
    
    /* create new state */
    state_t newState;
    newState.s_entryHI = id << ASID_SHIFT;
    newState.s_pc = newState.s_t9 = (memaddr) UPROCADDR;
    newState.s_sp = (int) STACKADDR;
    newState.s_status = ALLOFF | CBIT_IE_ON |FIELD_IM_ON| BIT_TE_ON | PBIT_USER_ON;

    /* create supplemental data structs */
    supStructs[id].sup_asid = id;
    /*set up general exception context*/
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) UexceptHandler;
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_stackPtr = (int) (topStack);
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | CBIT_IE_ON | FIELD_IM_ON | BIT_TE_ON;
        
    /*set up paper context*/
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) elPager;
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr =(int) (topStack+PAGESIZE);
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | CBIT_IE_ON | FIELD_IM_ON | BIT_TE_ON;
    /* initialize page table */
    int pg;
    for(pg=0; pg < MAXPGNUM; pg++) {
        supStructs[id].pageTable[pg].pe_entryHI = ALLOFF | ((UPROCSTRT + pg) << VPN_SHIFT) | (id << ASID_SHIFT);
        supStructs[id].pageTable[pg].pe_entryLO = ALLOFF | D_BIT_ON;
    }
    /*initialize the stack*/
    supStructs[id].pageTable[MAXPGNUM-1].pe_entryHI = ALLOFF | (PAGESTACK << VPN_SHIFT) | (id << ASID_SHIFT);

    /* call sys 1 */
    int status = SYSCALL(CREATEPROC, (int) &newState, (int) &(supStructs[id]), 0);
        
    /*if it did not go though okay kill it*/
    if(status != OK) {
        SYSCALL(KILLPROC, 0, 0, 0);
    }
}
