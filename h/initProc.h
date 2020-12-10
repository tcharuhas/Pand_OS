#ifndef initProc
#define initProc

/************************** INITPROC.H ******************************
*
*  The externals declaration file for the INITPROC
*  Written by Tyler C and Alex D
*
*	----------------------UPDATE ME---------------------------
*/

#include "../h/const.h"
#include "../h/types.h"
#include "/usr/lib/umps3/src/support/libumps/libumps.h"
#include "../h/sysSupport.h"
#include "../h/const.h"
int mainSem;
int devSem[NUMDEVS];

void exceptHandler();
void sysHandler(support_t* currSupport);

#endif