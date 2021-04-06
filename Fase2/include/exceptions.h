#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "../include/include.h"

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
    A Program Trap exception occurs when the Current Process attempts to performsome  illegal  or  undefined  action.
    This function take actions after a program trap exception occurs. 
*/
void programTrapHandler();


/*
    A TLB exception occurs whenμMPS3 fails in an attempt to translate a logicaladdress into its corresponding physical address.
    This function take actions after a TLB exception occurs. 
*/
void TLB_Handler();
#endif