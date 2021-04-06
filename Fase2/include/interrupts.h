#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../include/include.h"

/*
    interruptHandler is called by the exceptionshandler when a device or timer interrupt occurs either becuase of a previously initiated I/O request completes 
    or when either a Processor Local Timer (PLT) or the Interval Timermakes a 0x0000.0000⇒0xFFFF.FFFF transition.
    Depending on the device, the interrupt exception handler will perform a number of tasks.
*/
void interruptHandler();

#endif
