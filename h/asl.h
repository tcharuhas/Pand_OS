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

extern int insertBlocked (int *semAdd, pcb_PTR p);
extern void freeASL(semd_t* p);
extern semd_t* allocSem();
extern pcb_PTR removeBlocked (int *semAdd);
extern pcb_PTR outBlocked (pcb_PTR p);
extern pcb_PTR headBlocked (int *semAdd);
extern void initASL ();
int emptySem(semd_t* p);
int semFind(int* semAdd);
/***************************************************************/

#endif