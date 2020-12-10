/*********************************PCB.C*******************************
 *
 *	Maintains a NULL-terminated single, linearly linked list (using p_next field) of
 *  unused or NULL process control blocks whose head is pointed to by pcbFree_h.  Implements
 *	generic queue manipulation functions - with one of the parameters a pointer to a queue.
 *  The particular queue we will be maintaining is process queue (queue of pcbs), implemented as
 *  a doubly-circular linked list.  Finally, pcbs are organized into a process tree (tree of pcbs.)
 *  Implemented as follows: a parent pcb contains a pointer (p_child) to a NULL-terminated doubly,
 *  linearly linked list of its children pcbs.  Each child pcb contains a pointer to its parent (p_prnt)
 *
 *
 *	Authors: Tyler Charuhas, Alex DaLaet
 *	Last Modified: 9/8/20
 */


#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include <stdio.h>


pcb_PTR pcbFree_h;
int zPCB=0;

/* Insert the element pointed to by p onto the pcbFree list. */

void freePcb (pcb_PTR p) {

		if(emptyProcQ(pcbFree_h)) {
			pcbFree_h = p;
			return;
		}

		pcb_PTR temp = pcbFree_h;
		pcbFree_h = p;
		p->p_next = temp;
}



/*  Return NULL if the pcbFree list is empty. Otherwise, remove an element 
	from the pcbFree list, provide initial values for ALL of the pcbs fields 
	(i.e. NULL and/or 0) and then return a pointer to the removed element. Pcbs 
	get reused, so it is important that no previous value persist in a pcb when it gets reallocated. 

*/

pcb_PTR allocPcb() {

		if(emptyProcQ(pcbFree_h)) {
			return NULL;
		}

		/* Assign the current head to a temp, replace current head with p_next of temp */
 
		pcb_t* removedHead = pcbFree_h;
		pcbFree_h = pcbFree_h->p_next;

		removedHead->p_next = NULL;
		removedHead->p_prev = NULL;
		removedHead->p_child = NULL;
		removedHead->p_sib = NULL;
		removedHead->p_semAdd = NULL;

		return removedHead;
}


/*  Initialize  the  pcbFree  list  to  contain  all  the  elements  of  the static array of MAXPROCpcbs. 
	This method will be called only once during data structure initialization. 
*/

void initPcbs() {
	static pcb_t foo[20];

	int i = 0;

	while(i < 20) {
		freePcb(& foo[i]);
		i++;
	}
	
}

/* 	This method is used to initialize a variable to be tail pointer to a process queue.
	Return a pointer to the tail of an empty process queue; i.e. NULL.
*/

pcb_PTR mkEmptyProcQ () {
	return NULL;
}

/* Return TRUE if the queue whose tail is pointed to bytpis empty.Return FALSE otherwise. */

int emptyProcQ (pcb_PTR tp) {
	return(tp == NULL);
}

/* Insert the pcb pointed to by p into the process queue whose tail-pointer is pointed to by tp.  
   Note the double indirection throught p to allow for the possible updating of the tail pointer as well. 
*/

void insertProcQ (pcb_PTR *tp, pcb_t* p){
	/* if tail pointer points to null, then insert given pcb into the front of the list */
	if(emptyProcQ((*tp))) {
		*tp = p;
		(*tp)->p_next = (*tp);
		(*tp)->p_prev = (*tp);
		return;
		
	}


		p->p_next = (*tp)->p_next;
		(*tp)->p_next = p;
		p->p_prev=(*tp);
		p->p_next->p_prev = p;
		(*tp) = p;

}


/* Remove the first (i.e. head) element from the process queue whose tail-pointer is pointed to by tp.  
   Return NULL if the process queue was initially empty; otherwise return the pointer to the removed 
   element. Update the process queue’s tail pointer if necessary. 
*/


pcb_PTR removeProcQ (pcb_PTR *tp) {
	if(emptyProcQ(*tp)) {
		return NULL;
	}


	pcb_PTR head = (*tp)->p_next;
	if(head->p_prev == head) {
		head->p_prev = NULL;
		head->p_next = NULL;
		(*tp) = mkEmptyProcQ();
		return head;
	}

	pcb_PTR newHead = head->p_next;
	(*tp)->p_next = newHead;
    newHead->p_prev = *tp;
	head->p_next = NULL;
    head->p_prev = NULL;

	return head;
}


/* Remove the pcb pointed to by p from the process queue whose tail-pointer is pointed to by tp. Update the process 
   queue’s tail pointer if necessary.  If the desired entry is not in the indicated queue (an errorcondition), return NULL;
   otherwise, returnp.  Note that p can point to any element of the process queue. 
*/

pcb_PTR outProcQ (pcb_PTR *tp, pcb_t* p) {

	pcb_PTR head = headProcQ(*tp); /*will need to change back to headProcQ*/
            pcb_PTR curr = head;
            pcb_PTR prev = NULL;
        if (emptyProcQ(*tp)){
        	return NULL;
            
         }
            /*If the iteam is the head*/
        if (p == head){
            return(removeProcQ(tp));
          }
            /*is item the last item*/
        if (p == (*tp)){
                /*set the tail pointer to the previous item and then remove*/
                pcb_PTR temp = (*tp);
                *tp = (*tp)->p_prev;
                (*tp)->p_next =temp->p_next;
                temp->p_next->p_prev = (*tp);
                temp->p_next = NULL;
                temp->p_prev = NULL;
                return(temp);
            }
            
            /*Run though the list till find */
        	while (curr != p){
                prev = curr;
                curr = curr->p_next;
                /*if we get back to thr front of queue and didn't find it.*/
                if (curr == head && prev->p_next == head){
                    return(NULL);
                }
            }
   
           		prev->p_next = curr->p_next;
                curr->p_next->p_prev = prev;
                curr->p_next = NULL;
                curr->p_prev = NULL;
                return (curr);
                

	
}


/* Return a pointer to the first pcb from the process queue whose tail is pointed to by tp.  Do 
   not remove this pcb from the process queue.Return NULL if the process queue is empty. 
*/

pcb_PTR headProcQ (pcb_PTR tp) {
	if(emptyProcQ(tp)){
        return (NULL);
    }
    return(tp->p_next);
}


/* Return TRUE if the pcb pointed to by p has no children.  Return FALSE otherwise. */

int emptyChild (pcb_PTR p) {
	return(p->p_child == NULL);
}

/* Make the pcb pointed to by p a child of the pcb pointed to by prnt.*/

void insertChild (pcb_PTR prnt, pcb_PTR p) {
/*
		1. check if prnt has child, if so, assign child as a sib of p and assign p a prevSib of child.  Set prnt child to given p.  Keep children's prnt = current prn
		2. if prnt has no child, assign p to p_child and set p_sib & p_prevSib of p equal to null
*/

	if(!emptyChild(prnt)) {
		p->p_sib = prnt->p_child;
		prnt->p_child->p_prevSib = p;
		prnt->p_child = p;
		p->p_prnt = prnt;
		return;
	}

	prnt->p_child = p;
	prnt->p_child->p_prevSib = NULL;
	p->p_prnt = prnt;
	p->p_sib = NULL;



}

/* Make the first child of the pcb pointed to by p no longer a child of p.  Return NULL if initially there 
   were no children of p.  Otherwise,return a pointer to this removed first child pcb. 
*/

pcb_PTR removeChild (pcb_PTR p) {
/*
		1.  check if children exist.  if not, return NULL
		2.  set first child of pcb pointed to by p no longer a child. return pointer.
*/

	if(emptyChild(p)) {
		return NULL;
	}

	if(emptyProcQ(p->p_child->p_sib)) {
		pcb_t* temp = p->p_child;
		p->p_child = NULL;
		return temp;
	}

	pcb_t* temp = p->p_child;
	p->p_child->p_prevSib = NULL;
	p->p_child = p->p_child->p_sib;

	return temp;
}

/* Make the pcb pointed to by p no longer the child of its parent.  If the pcb pointed to by p has no parent, return NULL;
   otherwise, return p. Note that the element pointed to by p need not be the first child of its parent. 
*/

pcb_PTR outChild (pcb_PTR p) {
	/*
	if(p == NULL){
		return NULL;
	}

	if(p->p_prnt == NULL) {
		return NULL;
	}

	if(p == p->p_prnt->p_child) {
		if(p->p_sib == NULL && p->p_prevSib == NULL) {
			return(removeChild(p->p_prnt));
		}
		p->p_prnt->p_child = p->p_sib;
		p->p_sib->p_prevSib = NULL;
		
	}
	else if(p->p_sib == NULL){
		zPCB = 3;
		p->p_prevSib->p_sib = NULL;
	}

	else {
		zPCB = 2;
		p->p_prevSib->p_sib = p->p_sib;
		p->p_sib->p_prevSib = p->p_prevSib;
	}

	p->p_sib = NULL;
	p->p_prnt = NULL;
	p->p_prevSib = NULL;


return p;
	
*/
	if(p == NULL) {
		return NULL;
	}

	if(p->p_prevSib == NULL && p->p_sib == NULL && p == p->p_prnt->p_child) {
		p->p_prnt->p_child = NULL;
		p->p_prnt = NULL;
		return p;
	}

	if(p == p->p_prnt->p_child) {
		p->p_prnt->p_child = p->p_sib;
		p->p_sib->p_prevSib = NULL;
		p->p_sib = NULL;
		p->p_prnt = NULL;
		return p;
	}

	if(p->p_sib == NULL) {
		p->p_prevSib->p_sib = NULL;
		p->p_prevSib = NULL;
		p->p_prnt = NULL;
		return p;
	}

	if(p->p_prevSib != NULL && p->p_sib != NULL) {
		p->p_prevSib->p_sib = p->p_sib;
		p->p_sib->p_prevSib = p->p_prevSib;
		p->p_sib = NULL;
		p->p_prevSib = NULL;
		p->p_prnt = NULL;
		return p;
	}

	return NULL;
}

