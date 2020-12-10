/******************************EXCEPTIONS.C**********************************************
 * This file maintains exception handling functions for the kernel. 
 * These functions are invoked by the hardware whenever an exception
 * is raised, and handled accordingly.  The categories of exceptions
 * include TLB related issues, program traps, syscalls, and interrupts.
 * After each exception is handled, control is either passed back to the
 * current process or the scheduler is called.
 *
 * Written by: Tyler Charuhas & Alex Delaet
 *
 *
*/

#include "../h/const.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/initial.h"
#include "../h/types.h"
#include "/usr/local/include/umps3/umps/libumps.h"


/* 	exceptionHandler pulls the state from the BIOSDATA page and calculates the exception code
    via bitwise operations.  Depending on the code, we will branch to either the interruptHandler,
    the tlbHandler, or the sysCallHandler.  If no approprate branch is found, a program trap is invoked.
 
*/

void exceptionHandler() {
	state_t* prevState = (state_t*) BIOSDATAPAGE;
	int exception = (prevState->s_cause & FETCH_EXCCODE) >> SHIFT_CAUSE;


	/* Exception code 0 represents an interrupt */
	if(exception == INTERRUPT_IO) {

		interruptHandler();
	}

	/* Exception codes 1-3 represent a TLB error */
	if(exception <= INTERRUPT_TLB) {

		tlbHandler();
	}

	/* Exception code 8 represents a syscall */
	if(exception == INTERRUPT_SYSCALL) {

		sysCallHandler();

	} else {

		/* If none of the conditions are met, it is a prog trap */
		trap();
	}

}

/*	Syscall Handler determines the specific syscall and handles each one respectively
	when a SYSCALL trap is generated.  If the syscall is between 1-8 AND is executing in kernel
	mode, it is treated as a passed instruction and handled.  If it is between 1-8 AND it is not
	executing in kernel mode, a program trap is triggered.  If none of these conditions apply to the call,
	then we pass this exception to the support level or kill it.
*/
void sysCallHandler() {

	/* Loading in of the state which called the syscall, and loading in the syscall # */
	state_t * prevState;
	prevState = (state_t*) BIOSDATAPAGE;
	int syscall = prevState->s_a0;
	int kernelMode = (currentproc->p_s.s_status & PBIT_USER_ON); /* Are we in kernel mode? */

	/* If the syscall is between 1-8 and kernelMode is not on, a program trap is triggered */
	if (syscall >= CREATEPROC && syscall <= GETSUPPORT && kernelMode != OFF) {
		trap();
	}

	/* Storing the process state in the current process and incrementing pc so we don't loop */
	setState(prevState, &(currentproc->p_s));
	currentproc->p_s.s_pc += 4;

	/* Branch to the respective sycall passed to us. */
	switch (syscall) {
		default: trap(); /* default to a program trap unless a matching syscall is found */

		case CREATEPROC:
			createProc();
		case KILLPROC:
			terminateProc(currentproc);
			scheduler();
		case PASSEREN:
			P();
		case VERHOGEN:
			V();
		case IOWAIT:
			waitIO();
		case GETCPUTIME:
			getCPUTime();
		case CLOCKWAIT:
			PClockWait();
		case GETSUPPORT:
			getSupport();
	}
}

/* This function occurs whenever a program trap is invoked.  We either defer the exception handling to the support level or kill it */ 
void trap() {
	passupOrDie(GENERALEXCEPT);
}

/*  Whenever an exception is defered to the support level, this function is invoked.
	The support structure is grabbed from the current process.  If there is none, we terminate
	the process.  Otherwise, we pass it up to where the support level can reach.
*/
void passupOrDie(int exception) {
	support_t *supportStruct = currentproc->p_supportStruct;

	if ((supportStruct == NULL) || (supportStruct == OFF)) {
		terminateProc(currentproc);
		scheduler();
	}

	setState((state_t*) BIOSDATAPAGE, &(currentproc->p_supportStruct->sup_exceptState[exception]));
	LDCXT(currentproc->p_supportStruct->sup_exceptContext[exception].c_stackPtr, currentproc->p_supportStruct->sup_exceptContext[exception].c_status, currentproc->p_supportStruct->sup_exceptContext[exception].c_pc);
}


/* terminateProc recursively terminates the process and any progeny of the proccess.  All processes are freed to be reused. */
void terminateProc(pcb_t *terminate) {

	/* Recursive base case. Runs until no children are left. */
	while (!emptyChild(terminate)) {
		terminateProc(removeChild(terminate));
	}

	/* Is it the current process? */
	if (terminate == currentproc) {
		/* Is it the root process?*/
		if (terminate->p_prnt == NULL) {
			freePcb(terminate); 
			proccnt -= 1;
			return;
		}
		outChild(terminate);
	}
	/* Is it blocked on anyone? */
    if (terminate->p_semAdd == NULL) {
		outProcQ(&ready_h, terminate);
	}
	else {
		pcb_PTR removed = outBlocked(terminate);
		if (removed != NULL) {
			int *semAdd = removed->p_semAdd;
			/* Is the process it is blocked on between the first device and clock? */
			if (semAdd >= &devList[0] && semAdd <= &devList[CLOCK]) {
				softblockcnt -= 1;
			}
			else { 	
				(*semAdd) ++;
			}
		}
	}
	freePcb(terminate);
	proccnt -= 1; 
}


/* This function handles the processing of a TLB exception.  We either defer the exception to the support level or kill it */
void tlbHandler() {
	passupOrDie(PGFAULTEXCEPT);
}


/*  This function creates a child process of the current process.  The state of the child process is
	set to the state stored in a1.  Support data is stored in the child process from a2 if available.
	This child process is inserted on the ready queue and made a child of the current process.  Control
	is returned to the current process.
*/

void createProc() {

	pcb_PTR newProc = allocPcb();

	if (newProc == NULL) {
		currentproc->p_s.s_v0 = ERROR;
		return;
	}
	/* Set the new process state to state stored in a1 */
	state_t *toCheck = (state_t*) currentproc->p_s.s_a1;
	setState(toCheck, &newProc->p_s);

	/* Copying over the support struct ... */
	support_t *supportData = (support_t*) currentproc->p_s.s_a2;

	if (supportData != NULL || supportData != OFF) {
		newProc->p_supportStruct = supportData;
	}

	/* Incrementing process count, inserting on ready queue, making a child of current proccess, and returning control */
	proccnt++;
	insertProcQ(&ready_h, newProc);
	insertChild(currentproc, newProc);
	currentproc->p_s.s_v0 = OK;
	switchContext(currentproc);
}

/* Sycall 3.  Performs a P operation on the current process */
void P() {
	int *mutex = (int*) currentproc->p_s.s_a1;
	*mutex -= 1;
	if (*mutex < 0) {
		blockProc(mutex);
	}
	else {
		switchContext(currentproc);
	}
}

/* Sycall 4.  Performs a V operation on the current process */
void V() {
	/*verhogen */
	int *mutex = (int*) currentproc->p_s.s_a1;
	(*mutex) += 1;
	if ((*mutex) <= 0) {
		pcb_PTR temp = removeBlocked(mutex);
		if (temp != NULL) {
			insertProcQ(&ready_h, temp);
		}
	}
	switchContext(currentproc);
}

/* Syscall 5. */ 
void waitIO() {
	/* Get line number of interrupt and calculate devNum with expression */
	int lineNum = currentproc->p_s.s_a1;
	int devNum = (currentproc->p_s.s_a2) + ((lineNum - DISKINT)*DEVPERINT);
	/* Check if terminal */
	if (lineNum == TERMINT && currentproc->p_s.s_a3) {
		devNum = devNum + DEVPERINT;
	}

	/*decrement ... */
	devList[devNum]--;

	/* If interrupt has occured, pass control to the current process. */
	if (devList[devNum] >= 0) {
		switchContext(currentproc);
	}

	/* Otherwise increment softblock and block the process */
	softblockcnt++;
	blockProc(&(devList[devNum]));

}

/* This function loads the CPU Time then calculates the current time */
void getCPUTime() {
	/* Init current time */
	cpu_t TOD_current;
	STCK(TOD_current);
	TOD_current = (TOD_current - TOD_start) + currentproc->p_time;
	currentproc->p_s.s_v0 = TOD_current;

	switchContext(currentproc);

}

/* Syscall 7. */
void PClockWait() {

	/* Decrement clock sem */
	devList[CLOCK]--;

	/* If an interrupt has occured, switch back to original context */
	if (devList[CLOCK] >= 0)
	{
		switchContext(currentproc);
	}

	/* Block proccess */
	softblockcnt++;
	blockProc(&devList[CLOCK]);

}

/* Returns the support structure of the current process into v0. */
void getSupport() {
	currentproc->p_s.s_v0 = (int) currentproc->p_supportStruct;
	switchContext(currentproc);
}

/*  This function blocks the current process onto the semaphore specified. Timing stats are saved
	and the current process is set to null.  Control is passed to the scheduler.
*/

void blockProc(int *sem) {

	cpu_t TOD_stop;
	STCK(TOD_stop);
	currentproc->p_time = currentproc->p_time + (TOD_stop - TOD_start);
	/* Block proc */
	insertBlocked(sem, currentproc);
	currentproc = NULL;
	scheduler();

}