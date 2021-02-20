#ifndef ASL_H
#define ASL_H

#include "../include/pandos_const.h"
#include "../include/pandos_types.h"

/*Initialize the semdFree list to contain all the elements of the array
static semd t semdTable[MAXPROC]
This method will be only called once during data structure initializa-
tion.*/
void initASL();

/* Insert the pcb pointed to by p at the tail of the process queue as-
sociated with the semaphore whose physical address is semAdd and
set the semaphore address of p to semAdd. If the semaphore is cur-
rently not active (i.e. there is no descriptor for it in the ASL), allocate
a new descriptor from the semdFree list, insert it in the ASL (at the
appropriate position), initialize all of the fields (i.e. set s semAdd
to semAdd, and s procq to mkEmptyProcQ()), and proceed as
above. If a new semaphore descriptor needs to be allocated and the
semdFree list is empty, return TRUE. In all other cases return FALSE.
*/
int insertBlocked(int *semAdd, pcb_PTR p);

/* Search the ASL for a descriptor of this semaphore. If none is
found, return NULL; otherwise, remove the first (i.e. head) pcb from
the process queue of the found semaphore descriptor and return a
pointer to it. If the process queue for this semaphore becomes empty
(emptyProcQ(s procq) is TRUE), remove the semaphore de-
scriptor from the ASL and return it to the semdFree list. */
pcb_PTR removeBlocked(int *semAdd);

/* Remove the pcb pointed to by p from the process queue associated
with p’s semaphore (p→ p semAdd) on the ASL. If pcb pointed to by p
does not appear in the process queue associated with p’s
semaphore, which is an error condition, return NULL; otherwise, re-
turn p. */
pcb_PTR outBlocked(pcb_PTR p);

/* Return a pointer to the pcb that is at the head of the process queue
associated with the semaphore semAdd. Return NULL if semAdd is
not found on the ASL or if the process queue associated with semAdd
is empty. */
pcb_PTR headBlocked(int *);

#endif