#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include "include.h"
#include "sysSupport.h"
#include "initProc.h"

void initSwapStructs();
void atomicON();
void atomicOFF();
void pageFaultHandler();
#endif