#ifndef SYSUPPORT_H
#define SYSUPPORT_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "vmSupport.h"
#include "initProc.h"

void supportSyscallDispatcher(support_t *sup_struct);

void programTrap(int *sem);

void generalExceptionHandler();

#endif