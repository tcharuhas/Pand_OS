#ifndef sysSupport
#define sysSupport

/************************** SYSSUPPORT.H ******************************
*
*  The externals declaration file for the SysSupport
*  Written by Tyler C and Alex D
*
*	----------------------UPDATE ME---------------------------
*/

#include "../h/const.h"
#include "../h/types.h"
#include "/usr/lib/umps3/src/support/libumps/libumps.h"
#include "../h/initProc.h"
#include "../h/const.h"

void UexceptHandler();
void UsysHandler(support_t* currSupport);
void terminate();
void get_TOD(support_t* currSupport);
void writeToPrinter(support_t* currSupport);
void writeToTerm(support_t* currSupport);
void readFromTerm();

#endif