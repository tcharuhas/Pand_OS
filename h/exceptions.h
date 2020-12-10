#ifndef EXCEPTIONS
#define EXCEPTIONS

/************************** EXCEPTIONS.H ******************************
*
*  The externals declaration file for the Exception
*  Written by Tyler C and Alex D
*
*	----------------------UPDATE ME---------------------------
*/

#include "const.h"
#include "scheduler.h"
#include "interrupts.h"
#include "initial.h"
#include "types.h"
#include "/usr/local/include/umps3/umps/libumps.h"


extern void exceptionHandler();
extern void passupOrDie(int exception);
extern void terminateProc(pcb_t* terminate);
extern void blockProc();


/* Helper functions */
void sysCallHandler();
void trap();
void tlbHandler();
void createProc();
void P();
void V();
void waitIO();
void getCPUTime();
void PClockWait();
void getSupport();

/************************************************************************/

#endif
