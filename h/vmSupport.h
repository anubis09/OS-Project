#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "pandos_const.h"
#include "sysSupport.h"
#include "initProc.h"

/*
    Initializes all the support level data structures, such as the swap pool
    table, and the swap pool semaphore.
*/
void initSwapStructs();

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

*/
void pageFaultHandler();
#endif