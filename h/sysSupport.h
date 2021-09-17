#ifndef SYSUPPORT_H
#define SYSUPPORT_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "vmSupport.h"
#include "initProc.h"

/*
    The Support Level general exception handler will process all passed up 
    non-TLB exceptions:
    -All SYSCALL (SYSCALL) exceptions numbered 9 and above.
    -All Program Trap exceptions.
*/
void generalExceptionHandler();

/*
    The Support Level’s Program Trap exception handler is to terminate the process
    in an orderly fashion; perform the same operations as a SYS9 request.
    Asid is passed so that we can call swapCleanUp, and eliminate extraneous writes to
    the backing store.
*/
void programTrap(int asid);

#endif