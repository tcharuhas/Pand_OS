#ifndef INTERRUPTS
#define INTERRUPTS

/************************** INTERRUPTS.H ******************************
*
*  The externals declaration file for the Interrupts
*  Written by Tyler C and Alex D
*
*	
*/

#include "const.h"
#include "pcb.h"
#include "types.h"
#include "initial.h"
#include "exceptions.h"
#include "scheduler.h"
#include "/usr/local/include/umps3/umps/libumps.h"

extern void interruptHandler();
void Handle_PLT ();
void Handle_Timer();
void Handle_IO(int lineNum);
int terminal_interruptH(int *devSem);
/**********************************************************************/

#endif
