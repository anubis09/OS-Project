#ifndef PCB_H
#define PCB_H

#include "pandos_const.h"
#include "pandos_types.h"

/*Allocation and Deallocation of pcbs*/

/*Initialize the pcbFree list to contain all the elements of the
static array of MAXPROC pcbs. This method will be called only
once during data structure initialization. */
void initPcbs();

/*pcbs which are no longer in use can be returned to the pcbFree list by using
the method*/
void freePcb(pcb_PTR p);

/*Return NULL if the pcbFree list is empty. Otherwise, remove
an element from the pcbFree list, provide initial values for ALL
of the pcbs fields (i.e. NULL and/or 0) and then return a pointer
to the removed element.*/
pcb_PTR allocPcb();

/*Process Queue Maintenance*/

/*Return a pointer to the tail of an empty process queue*/
pcb_PTR mkEmptyProcQ();

/*Return TRUE if the queue whose tail is pointed to by tp is empty.
Return FALSE otherwise.*/
int emptyProcQ(pcb_PTR tp);

/*Insert the pcb pointed to by p into the process queue whose tail-
pointer is pointed to by tp.*/
void insertProcQ(pcb_PTR *tp, pcb_PTR p);

/*Remove the first (i.e. head) element from the process queue whose
tail-pointer is pointed to by tp. Return NULL if the process queue
was initially empty; otherwise return the pointer to the removed ele-
ment.*/
pcb_PTR removeProcQ(pcb_PTR *tp);

/*Remove the pcb pointed to by p from the process queue whose tail-
pointer is pointed to by tp. If the desired entry is not in the indicated queue (an error
condition), return NULL; otherwise, return p*/
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p);

/*Return a pointer to the first pcb from the process queue whose tail
is pointed to by tp. Do not remove this pcbfrom the process queue.
Return NULL if the process queue is empty.*/
pcb_PTR headProcQ(pcb_PTR);

/*Process Tree Maintenance*/

/*Return TRUE if the pcb pointed to by p has no children. Return
FALSE otherwise.*/
int emptyChild(pcb_PTR p);

/*Make the pcb pointed to by p a child of the pcb pointed to by prnt.*/
void insertChild(pcb_PTR prnt, pcb_PTR p);

/*Make the first child of the pcb pointed to by p no longer a child of
p. Return NULL if initially there were no children of p. Otherwise,
return a pointer to this removed first child pcb.*/
pcb_PTR removeChild(pcb_PTR p);

/*Make the pcb pointed to by p no longer the child of its parent. If
the pcb pointed to by p has no parent, return NULL; otherwise, return
p.*/
pcb_PTR outChild(pcb_PTR p);

#endif