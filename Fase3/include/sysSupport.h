#ifndef SYSUPPORT_H
#define SYSUPPORT_H

#include "../include/include.h"

void supportSyscallDispatcher(support_t *sup_struct);

void programTrap();

void generalExceptionHandler();

#endif