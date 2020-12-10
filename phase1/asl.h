#ifndef ASL
#define ASL

/************************** ASL.H ******************************
*
*  The externals declaration file for the Active Semaphore List
*    Module.
*
*  Written by Mikeyg
*/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"

static int MAXINT = 2147483647;
extern int insertBlocked (int *semAdd, pcb_PTR p);
extern void freeASL(semd_t* p);
extern semd_t* allocSem();
extern pcb_PTR removeBlocked (int *semAdd);
extern pcb_PTR outBlocked (pcb_PTR p);
extern pcb_PTR headBlocked (int *semAdd);
extern void initASL ();
static semd_t* semFind(int *semAdd);
static int emptySem(semd_t* p);
/***************************************************************/

#endif
