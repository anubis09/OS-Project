#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "pandos_const.h"
#include "sysSupport.h"
#include "initProc.h"

memaddr swapStart;

/*
    Initializes all the support level data structures, such as the swap pool
    table, and the swap pool semaphore.
*/
void initSwapStructs();

/*
    Given the asid of a process, mark all of his frames as unoccupied.
*/
void swapCleanUp(int asid);

/*
    Disables interrupts, so that operations done after this function
    are atomical.
*/
void atomicON();

/*
    Enables interrupts.
*/
void atomicOFF();

/*
    Handles page fault exceptions, passed up by the nucleus.
*/
void pageFaultHandler();

void flashOperation(int asid, unsigned int page, memaddr pfn, int command);
#endif