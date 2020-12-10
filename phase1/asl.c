/*********************************ASL.C*******************************
 *
 *  Maintains a NULL-terminated single, linearly linked list (using s_next field) of
 *  semaphore descriptors whose head is pointed to by semd_h.  This is the active semaphore
 *  list and is sorted in ascending order by the s_semAdd field.  Maintains a second NULL-terminated
 *  single, linearly linked list of unused semaphores.  Pointed to by semdFree_h
 *  
 *  
 *
 *
 *  Authors: Tyler Charuhas, Alex DaLaet
 *  Last Modified: 9/8/20
 */



#include "../h/const.h"
#include "../h/types.h"
#include "../h/asl.h"
#include "../h/pcb.h"





HIDDEN semd_t *semd_h,*semdFree_h, *foundSem, firstDummy, lastDummy;


/* Returns if a given semaphore is NULL */
int emptySem(semd_t* p) {
    return (p == NULL);
}

/* Insert the semaphore pointed to by p onto the semdFree list */

void freeASL(semd_t* p) {
	if(emptySem(p)) {
			semdFree_h = p;
			return;
		}

		semd_t* temp = semdFree_h;
        semdFree_h = p;
        semdFree_h->s_next = temp;
}

/*  Return NULL if the semdFree list is empty. Otherwise, remove an element 
	from the semdFree list, provide initial values for ALL of the pcbs fields 
	(i.e. NULL and/or 0) and then return a pointer to the removed element. 
*/

semd_t* allocSem() {

	if(emptySem(semdFree_h)) {
		return NULL;
	}

	semd_t* temp = semdFree_h;
	semdFree_h = temp->s_next;

	temp->s_next = NULL;
	temp->s_procQ = mkEmptyProcQ();

	return temp;
}


/*  Insert the pcb pointed to by p at the tail of the process queue associated with the semaphore whose physical address is semAdd
	and set the semaphore address of p to semAdd.  If the semaphore is currently not active (i.e. there is no descriptor for it in 
	the ASL), allocate a new descriptor from the semdFree list, insert it in the ASL (at the appropriate position), initialize all 
	of the fields (i.e.  set s_semAdd to semAdd, and s_procq to mkEmptyProcQ()), and proceed as above.  If a new semaphore descriptor 
	needs to be allocated and the semdFree list is empty, return TRUE. In all other cases return FALSE.*/

 int insertBlocked (int *semAdd, pcb_PTR p) {
    
    if(!(semFind(semAdd))){
        semd_t* allocatedSem = allocSem();
        
        if((emptySem(allocatedSem))){
            return TRUE;
        }
        
        allocatedSem->s_next = foundSem->s_next;
        allocatedSem->s_semAdd = semAdd;
        allocatedSem->s_procQ = mkEmptyProcQ();
        
        foundSem->s_next = allocatedSem;
        p->p_semAdd = semAdd;
        insertProcQ(&allocatedSem->s_procQ, p);
        return FALSE;
    }
    
    p->p_semAdd = semAdd;
    insertProcQ(&foundSem->s_next->s_procQ, p);
    return FALSE;
 }


/*  Search the ASL for a descriptor of this  semaphore. If  none  is found, return NULL; otherwise, remove the first (i.e. head) pcb
	from the process queue of the found semaphore descriptor and return a pointer to it. If the process queue for this semaphore becomes 
	empty(emptyProcQ(s_procq) is  TRUE),  remove  the  semaphore  de-scriptor from the ASL and return it to the semdFree list. 
*/

 pcb_PTR removeBlocked (int *semAdd) {
     
     if (!(semFind(semAdd))){         
         return NULL;
     }
     
    
    pcb_PTR removedPcb = removeProcQ(&foundSem->s_next->s_procQ);
     
    if(emptyProcQ(foundSem->s_next->s_procQ)){
        
        semd_t* temp = foundSem->s_next;
        foundSem->s_next = temp->s_next;
        temp->s_next = NULL;
                
        freeASL(temp);
    }
        return(removedPcb);

 }

/*  Remove the pcb pointed to by p from the process queue associated with p’s semaphore (p→psemAdd) on the ASL. If pcb pointed
	to by p does  not  appear  in  the  process  queue  associated  with p’s semaphore, which is an error condition, return NULL;
	 otherwise, return p. 
*/

 pcb_PTR outBlocked (pcb_PTR p) {
     
     if(!(semFind(p->p_semAdd))){
         return NULL;
     }
     
     pcb_PTR foundPcb = outProcQ(&foundSem->s_next->s_procQ,p);
     
     if(emptyProcQ(foundPcb)){
         return(NULL);
     }
     
     return(foundPcb);
 }


/* Return a pointer to the pcb that is at the head of the process queue associated with the semaphore semAdd. Return NULL if semAdd 
   is not found on the ASL or if the process queue associated with semAdd is empty. 
*/

 pcb_PTR headBlocked (int *semAdd) {
     
     if (!(semFind(semAdd))){
         return (NULL);
     }
     
     if(emptyProcQ(foundSem->s_next->s_procQ)){
         return(NULL);
     }
     return headProcQ(foundSem->s_next->s_procQ);


 }

/*  Initialize the semdFree list to contain all the elements of the array static semdt semdTable[MAXPROC]. This method will 
	be only called once during data structure initialization.  Creates two dummy nodes for the ASL for easy traversal */

 void initASL () {
 	static semd_t semdTable[MAXPROC];
    int i;
 	for(i = 0; i < MAXPROC; i++) {
 		freeASL(& semdTable[i]);
 	}

 	firstDummy.s_semAdd = 0;
 	firstDummy.s_next = &lastDummy;

    
 	lastDummy.s_semAdd = &MAXINT;
 	lastDummy.s_next = NULL;

 	semd_h = &firstDummy;
    
 }
 
/*	Given a semAdd, traverse the ASL to search for the semaphore associated with it.  This function is utilized
	by removeBlocked, insertedBlocked, outBlocked, and headBlocked.  To allow for easy pointer re-alignment during a 
	semaphore removal, this function returns TRUE or FALSE. TRUE if the semaphore is found and FALSE if not.
	If the semaphore is not found, foundSem (a global var) will be set to the previous node for easy insertion to where 
	the looked for semaphore shoud be. If the semaphore is found foundSem will be set to the previous semaphore.
	2147483647
*/

int semFind(int *semAdd) {
    
    semd_t *prev = semd_h;
    semd_t *curr = prev->s_next;
    
    /* If the list is empty*/
    if(curr->s_semAdd == &MAXINT){
        foundSem = prev;
        return(FALSE);
    }
    
    while(*curr->s_semAdd <= *semAdd){
        if(curr->s_semAdd == semAdd){
            foundSem = prev;
            return(TRUE);
        }
            prev = prev->s_next;
            curr = curr->s_next;
    }
    /* Could not find in list */
    foundSem = prev;
    return(FALSE);

}
