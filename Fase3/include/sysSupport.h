#ifndef SYSUPPORT_H
#define SYSUPPORT_H

#include "../include/include.h"
#include "../include/vmSupport.h"

void supportSyscallDispatcher(support_t *sup_struct);

void programTrap(int asid);

void generalExceptionHandler();

#endif