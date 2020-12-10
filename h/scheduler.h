#ifndef SCHEDULER
#define SCHEDULER

/************************** SCHEDULER.H ******************************
*
*  The externals declaration file for the Scheduler
*  Written by Tyler C and Alex D
*
*	
*/
#include "const.h"
#include "pcb.h"
#include "types.h"
#include "initial.h"
#include "exceptions.h"
#include "interrupts.h"
#include "/usr/local/include/umps3/umps/libumps.h"

extern void scheduler();
extern void switchContext(pcb_t* toSwitch);
extern void setState(state_t* stateToCopy, state_t* stateToFill);

/*********************************************************************/

#endif
