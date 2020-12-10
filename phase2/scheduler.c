/* ****************SCHEDULER.C**********************************
 *
 * This file is used to control the follow of processes, more specifically
 * processes are transitioned from ready to running. A new
 * process is grabbed from the ready queue or the state of the current
 * process is loaded in via LDST.  Both local time and interval time
 * are set here.  
 * 
 * 
 * Written by: Alex D & Tyler C
 * 
 * 
 */


#include "../h/const.h"
#include "../h/pcb.h"
#include "../h/types.h"
#include "../h/initial.h"
#include "../h/exceptions.h"
#include "../h/interrupts.h"

#include "/usr/local/include/umps3/umps/libumps.h"


/*
  Scheduler dispatches the next job up in the ready queue.  Starvation is guaranteed to be
  avoided by implementing a preemptive round-robin scheduling algorithm.  If there are no jobs 
  left on the queue, the appropriate BIOS action is taken.  
*/
void scheduler() {
    /*pull the head off of the ready queue*/
	pcb_t* removed = removeProcQ(&ready_h);

	if(removed != 0) { 
        /*start the clock and timer*/
		STCK(TOD_start);
		setTIMER(LOCALTIME);
		switchContext(removed);
    
	} 
	
	/*if we don't have a process on the ready queue... */
	else {
        
        /* HALT BIOS */
		if(proccnt == 0) {
			HALT();
		}
        
        /*enter wait state*/
		if(proccnt > 0 && softblockcnt > 0) { 
            
			currentproc = NULL;
            /*set PLT to very large number. */
			setTIMER(DISABLE); 
            /*turn on interrupts, timer interrupts and the interrupt fields*/
			setSTATUS(ALLOFF | FIELD_IM_ON | CBIT_IE_ON | BIT_TE_ON ); 
			WAIT();


		}
        
        /* deadlock, panic */
		if(proccnt > 0 && softblockcnt == 0) {
			PANIC();
		}
	}
}

/*
    switchContext is passed a PCB.  The currentprocess
    is set to that PCB and the state associated with it
    is passed to the BIOS service LDST.
 */
void switchContext(pcb_t* switchTo) {
	currentproc = switchTo;
	LDST(&switchTo->p_s);
}

/*
  setState takes two states: one to copy, and one to fill with that copy.
  All 31 registers are looped through and copied over, then the remaining
  4 are copied.  A shallow copy is avoided by passing the states in as pointers
  and looping through them.

*/
void setState(state_t* stateToCopy, state_t* stateToFill) {

	/* loop through to copy all the registers */
	int i = 0;
	while(i < STATEREGNUM) {
        
		stateToFill->s_reg[i] = stateToCopy->s_reg[i];
		i++;
        
	}
	/* set the state fields */
	stateToFill->s_status = stateToCopy->s_status;
	stateToFill->s_cause = stateToCopy->s_cause;
	stateToFill->s_pc = stateToCopy->s_pc;
	stateToFill->s_entryHI = stateToCopy->s_entryHI;

}

