#ifndef vmSupport
#define vmSupport

/************************** VMSUPPORT.H ******************************
*
*  The externals declaration file for the VMSUPPORT
*  Written by Tyler C and Alex D
*
*	----------------------UPDATE ME---------------------------
*/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/initial.h"
#include "../h/initProc.h"
#include "/usr/lib/umps3/src/support/libumps/libumps.h"


void kill();
void initTLB();
void toggleInts(int status);
void uTLB_RefillHandler();
void elPager();
void programTrap();


#endif