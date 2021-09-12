#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include "../include/include.h"
#include "../include/sysSupport.h"

#define FLASH 0
#define PRINTER 1
#define TERMINAL 2
#define SUPDEVSEMNUM 3

void freeME(int asid);
void atomicON();
void atomicOFF();

mutualExclusion_Semaphore supportDeviceSemaphore[3];

#endif