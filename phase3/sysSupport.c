/*****************************sysSupport.c***********************************
 * 
 * sysSupport is home to our User mode general exception handle and the user
 * mode syscall handler which hands syscalls 9-13 and above. The syscall
 * functions for syscalls that are terminate, get time of day clock, write to
 * printer, write to terminal, and finaly read from terminal.
 * 
 *
 * 
 *  Written by: Tyler Charuhas and Alex Delaet
 */



#include "../h/const.h"
#include "../h/types.h"
#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "/usr/lib/umps3/src/support/libumps/libumps.h"

/*
 * 
 * Our UexceptHandler is what is called in the pass up context of a user process.
 * it cheks the cause to see if it was a syscall and if so passes the sys onto our
 * user syscall handler. Otherways it terminates it in a programtrap from vmSupport.
 * 
 */
void UexceptHandler() {

	/* pull in the current support and cause */
	support_t* currentSupport = (support_t*) SYSCALL(GETSUPPORT,0,0,0);
    /*get cause*/
	int cause = ((currentSupport->sup_exceptState[GENERALEXCEPT].s_cause) & FETCH_EXCCODE) >> SHIFT_CAUSE;

	/* if it is a syscall, handle it. */
	if(cause == SYS) {
		UsysHandler(currentSupport);
	}
	/* it's not a syscall so we treat it as a trap */
	programTrap();
}


/*
 * 
 * Our user syscall handler checks for the type of syscall that was called, and then
 * runs that function. If the type of syscall that was called is not 9-13 then a programTrap
 * is called. After the syscall has been handled we break out of the switch statmemnt and then
 * load the state of state that causesd the exeption.
 * 
 */
void UsysHandler(support_t* currSupport) {
	/* increment pc to prevent looping, read in syscall */
	currSupport->sup_exceptState[GENERALEXCEPT].s_pc += 4;
    /*get the syscall*/
	int syscall = currSupport->sup_exceptState[GENERALEXCEPT].s_a0;
	/* branch respectively ... */
	switch(syscall) {
		case TERMINATE: terminate(); break;
		case GETTOD: get_TOD(currSupport); break;
		case WRITEPRINTER: writeToPrinter(currSupport); break;
		case WRITETERM: writeToTerm(currSupport); break;
		case READTERM: readFromTerm(currSupport); break;
		default: programTrap();
	}

	LDST(&(currSupport->sup_exceptState[GENERALEXCEPT]));
}


/* 
 * 
 * This is the function for sys 9. Calling this 
 * fuction runs the kill function from VM support
 * 
 */
void terminate() {
	kill(NULL);
}


/* 
 * 
 * This is the function for sys 10. Calling this
 * fuction returns the current time of the 
 * Time of Day clock  to v0.
 * 
 */
void get_TOD(support_t* currSupport) {
	
	cpu_t currTOD;
	STCK(currTOD);
	currSupport->sup_exceptState[GENERALEXCEPT].s_v0 = currTOD;
}


/* 
 * 
 * This is the function for sys 11. Calling this
 * fuction will cause the current process to be 
 * suspended until a line of output (string of 
 * characters) has been transmitted to the printer
 * device associated with the proccess. After that
 * is done the number of characters that were 
 * printed will be return in v0.
 * 
 */
void writeToPrinter(support_t* currSupport) {

    /* assigning varrables to be used later */
    /*get the varbiles from syscall state*/
	char *virtAddr = (char *)currSupport->sup_exceptState[GENERALEXCEPT].s_a1;
	int length = currSupport->sup_exceptState[GENERALEXCEPT].s_a2;
    /*get printer Num*/
	int printerNum = currSupport->sup_asid-1;
    /*get the printer's semaphore*/
	int printerSem = ((PRNTINT-DISKINT)*DEVPERINT)+printerNum;
    /* get teh device regs*/
	devregarea_t* devRegs = (devregarea_t*) RAMBASEADDR;
    /*general vars*/
	int error = FALSE;
	int i=0;
	int status;

	/* 
     * if the address of the string is below the UProc level, and the length does not meet
     * our constraints, programTrap. 
     */
	if((int) virtAddr < KUSEG || (length <= 0) || (length > MAXSTRLENGTH)) {
		programTrap();
	}
    /* get mutex*/
	SYSCALL(PASSEREN,(int) &devSem[printerSem],0,0);
    
    /*while we haven't errored out and are still not at the length send the char*/
	while(i<length && error == FALSE) {
        /*send the chars then wait*/
		devRegs->devreg[printerSem].d_data0 = *virtAddr;
		devRegs->devreg[printerSem].d_command = CHAR_TRANSMIT;
		status = SYSCALL(IOWAIT, PRNTINT, printerNum, 0);
        
        /*if the the send is good keep going*/
		if(status == CHAR_TRANSOK) {
			i++;
            virtAddr++;
		} 
		else {
			error = TRUE;
		}
	}
	/*end mutex*/
	SYSCALL(VERHOGEN,(int) &devSem[printerSem],0,0);
    
    /*save number of sent characters to v0*/
	currSupport->sup_exceptState[GENERALEXCEPT].s_v0 = i;

}


/* 
 * 
 * This is the function for sys 12. Calling this
 * fuction pauses the user process and then will 
 * transmite characters to the Uprocs assignined
 * terminal. After the characters haven successfully
 * transmitted the number that have been are stored
 * in v0.
 * 
 */
void writeToTerm(support_t* currSupport) {

	char *virtAddr = (char *)currSupport->sup_exceptState[GENERALEXCEPT].s_a1;
	int length = currSupport->sup_exceptState[GENERALEXCEPT].s_a2;
	int termNum = currSupport->sup_asid-1;
	int termSem = ((TERMINT-DISKINT)*DEVPERINT)+termNum;
	devregarea_t* devRegs = (devregarea_t*) RAMBASEADDR;
	int error = FALSE;
	int i=0;
	int status;

	/* if the address of the string is below the UProc level, and the length does not meet
	   our constraints, programTrap.
	*/
	if((int) virtAddr < KUSEG || (length < 0) || (length > MAXSTRLENGTH)) {
		programTrap();
	}
    
    /*get mutex*/
	SYSCALL(PASSEREN,(int) &devSem[termSem],0,0);
    
    /*while we haven't errored out and are still not at the length send the char*/
	while(i<length && error == FALSE) {
        /*send the characters then wait*/
		devRegs->devreg[termSem].t_transm_command = *virtAddr << BYTE_LENGTH | CHAR_TRANSMIT;
		status = SYSCALL(IOWAIT, TERMINT, termNum, 0);
        
        /*if the the send is good keep going*/
		if((status & STATUSBIT) == CHAR_TRANSOK) {
			i++;
            virtAddr++;
		} 
		else {
			error = TRUE;
		}
	}
	
	/*end mutex*/
	SYSCALL(VERHOGEN,(int) &devSem[termSem],0,0);
    
    /* if we errored, negate the number of sent chars so we know*/
	if(error) {
		i = 0 - (status & STATUSBIT);
	}
	/*store number of sent chars to v0 */
	currSupport->sup_exceptState[GENERALEXCEPT].s_v0 = i;

}


/* 
 * 
 * This is the function for sys 12. Calling this
 * fuction pauses the current Uproc until a line
 * from the terminal is inputted by the user.
 * The data should be given to the fuction in a1
 * and then after the read is finished the characters
 * read will be stored in v0.
 * 
 */
void readFromTerm(support_t* currSupport) {
    /*get the address from a1 */
	char *virtAddr = (char *)currSupport->sup_exceptState[GENERALEXCEPT].s_a1;
    /*get the terminal number and semaphore */
	int termNum = currSupport->sup_asid-1;
	int termSem = ((TERMINT-DISKINT)*DEVPERINT)+termNum;
    /*get a devReg */
	devregarea_t* devRegs = (devregarea_t*) RAMBASEADDR;
    /*general vars*/
	int error = FALSE;
	int completed = FALSE;
	int i=0;
	int status;
    
    /*if the address is outside of the KUSEG then kil the process */
	if((int)virtAddr < KUSEG) {
		kill(NULL);
	}

    /* get mutex*/
	SYSCALL(PASSEREN,(int) &devSem[termSem+DEVPERINT],0,0);

    /*if we're not done and we haven't errored out keep going*/
	while((completed == FALSE) && (error == FALSE)) {
        /*send the command to get the char and wait*/
		devRegs->devreg[termSem].t_recv_command = CHAR_TRANSMIT;
		status = SYSCALL(IOWAIT, TERMINT, termNum, 1);
        /*if we read okay then move on to the next char*/
		if((status & STATUSBIT) == CHAR_TRANSOK) {
			i++;
            /* shifts to next byte*/
			*virtAddr = status >> BYTE_LENGTH;
			virtAddr++;
            /*if we're at the end (newline or whatever ASCII char you wish to end on) we're good*/
			if((status >> BYTE_LENGTH) == ASCIINEWLINE) {
				completed = TRUE;
			}
		} else {
			error = TRUE;
		}
		
	}
	/*end mutex*/
	SYSCALL(VERHOGEN,(int) &devSem[termSem+DEVPERINT],0,0);
    
    /*if we errored get the negation of the read chars*/
	if(error) {
		i = 0 - (status & STATUSBIT);
	}
	/*send the number of read chars to v0*/
	currSupport->sup_exceptState[GENERALEXCEPT].s_v0 = i;
}
