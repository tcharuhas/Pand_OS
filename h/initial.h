#ifndef INITIAL
#define INITIAL

/************************* INITIAL.H *****************************
*
*  The externals declaration file for the Initial File of Phase 2
*  Written by Tyler C and Alex D
* 	-----------------UPDATE ME----------------------
*/


#include "const.h"
#include "types.h"
#include "pcb.h"
#include "asl.h"
#include "scheduler.h"
#include "/usr/local/include/umps3/umps/libumps.h"

extern memaddr calculateRAMTOP();

extern void test();
extern void uTLB_RefillHandler();

extern int 	proccnt, softblockcnt;
extern cpu_t	TOD_start;
extern int 	devList[NUMDEVS];
extern pcb_PTR ready_h, currentproc;

void createProcess();
/******************************************************************/

#endif
