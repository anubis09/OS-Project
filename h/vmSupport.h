#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "pandos_const.h"
#include "sysSupport.h"
#include "initProc.h"

void initSwapStructs();
void atomicON();
void atomicOFF();
void pageFaultHandler();
#endif