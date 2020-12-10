/*************************INITIAL.C**********************************************
 *	
 *	This file contains the intitialization routines of the OS.  Data structures,
 *  memory locations, and data for the emulator are initialized and maintained via   
 *  the nucleus.  A single process is created then placed on the ready queue, then we
 *  jump to the scheduler. The ready queue serves as a queue of jobs for PANDOS to perform.
 *
 *
 *	Written by: Tyler Charuhas & Alex DeLaet
 *
 *
 */

#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "/usr/local/include/umps3/umps/libumps.h"



/* Global nucleus variable declaration */
int 	proccnt, softblockcnt;
cpu_t	TOD_start;
int 	devList[NUMDEVS];
pcb_PTR ready_h, currentproc;


/* Start of the Nucleus initialization */
void main() {

	/* PASSUPVECTOR Initialization */
	passupvector_t *nucleus;
	nucleus = (passupvector_t*) PASSUPVECTOR;
	nucleus->tlb_refll_handler = (memaddr) uTLB_RefillHandler;
	nucleus->tlb_refll_stackPtr = (memaddr) STACK;
	nucleus->execption_handler = (memaddr) exceptionHandler;
	nucleus->exception_stackPtr = (memaddr) STACK;

	/* Initialization of processes & ready queue.  Ready queue and the current process are both defaulted to NULL */
	initPcbs();
	initASL();
	proccnt = softblockcnt = 0;
	ready_h = mkEmptyProcQ();
	currentproc = NULL;

	/* All I/O devices are set to 0, and all clocks are set to 0*/
	/* each terminal has two semaphores */
	devList[CLOCK] = 0;	
	int i;
	for(i = 0; i < (NUMDEVS-1); i++) {
		devList[i] = 0;
	}

	/* Load the pseudo clock with interval time (INTERVALTIME = 100ms) */
	/* Create the first process so PANDOS can run the test file */
	/* Call the scheduler to start running the test file */
	LDIT(INTERVALTIME);
	createProcess();
	scheduler();

}

/*	createProcess allocates a process for PANDOS that contains the address of the test function.
 *  This newly created process gives us the capability to run through the test file
 *
*/

 void createProcess() {	

	pcb_PTR temp = allocPcb();

	if(temp == NULL) {
		PANIC();
	}

	proccnt++;

	memaddr calcRam = calculateRAMTOP(); /* Get the RAMTOP address so we can start using the stack from the top to bottom*/
	temp->p_s.s_sp =  calcRam; 

	/* Modifying the newly allocated process state to run the test function, then inserting on the ready queue for processing */
	temp->p_s.s_pc = (memaddr) test; 
	temp->p_s.s_t9 = (memaddr) test;
	temp->p_s.s_status = ALLOFF | PBIT_IE_ON | FIELD_IM_ON | BIT_TE_ON;
	temp->p_time = 0;
	temp->p_semAdd = NULL;
	temp->p_supportStruct = NULL;
    temp->p_prnt = NULL;
	insertProcQ(&ready_h, temp);
}



/* Calculates the top of the RAM. */
memaddr calculateRAMTOP() {
	return *(int*)RAMBASEADDR + *(int*)RAMBASESIZE;
}
