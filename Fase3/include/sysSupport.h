#ifndef SYSUPPORT_H
#define SYSUPPORT_H

#include "include.h"
#include "vmSupport.h"
#include "initProc.h"

void supportSyscallDispatcher(support_t *sup_struct);

void programTrap(int *sem);

void generalExceptionHandler();

#endif