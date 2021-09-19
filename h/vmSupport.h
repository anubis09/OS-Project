#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "pandos_const.h"
#include "sysSupport.h"
#include "initProc.h"

/*
    the starting address of the swap pool frames.
*/
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

/*
    This functions is for writing or reading from a flash device.
    Asid is the asid of the process for getting the right FLASH device.
    Page is the device block number.
    Pfn is the starting physical address of the 4k block.
    Command is for the proper operation to perform ( READ WRITE)
*/
void flashOperation(int asid, unsigned int page, memaddr pfn, int command);
#endif