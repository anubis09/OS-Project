#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "../include/include.h"
#include "scheduler.h"
#include "initialize.h"

/*
    This function is the Nucleous exceptions handler. It's called after each and
    every exceptions ( exclusive of TLB-Refill events).
    detect which type of exception happend, and pass control to the defined handler.
*/
void exceptionshandler();

/*
    syscallDispatcher is called by the exceptionshandler when a system call is requested.
    Determines if the precess requesting the syscall was in kernel mode, if so check which type of syscall
    was requested, and passes control to the right handler. if the process was in usermode, a program trap exception is raised

*/
void syscallDispatcher();

/*
    For SYSCALL exceptions numbered 9 and above, Program Trap and TLB exceptions,
    the Nu-cleus will take one of two actions depending on whether the current process 
    was provided a non-NULL value for its Support Structure pointer when it was created:
    1) If the current process's support structureis NULL, then the exception should be handled as a SYS2.
    2) If the Current Process’spsupportStructis non-NULL. The handlingof the exception is “passed up.”
*/
void passUp_Die(int);
#endif