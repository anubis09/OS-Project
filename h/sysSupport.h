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
    It's an error to kill a process that is actually holding a mutual exclusion on a 
    Support Level semaphore, so we takes as input a semaphore pointer to V'ed (NULL if
    the process is not holding a mutual exclusion).
*/
void programTrap(int *sem);

#endif