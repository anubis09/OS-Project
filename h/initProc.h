#ifndef INITPROC_H
#define INITPROC_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "pandos_const.h"
#include "vmSupport.h"

#define FLASH 0
#define PRINTER 1
#define TERMINAL 2
#define SUPDEVSEMNUM 32

/*
    0-7 FLASH
    8-15 PRINT
    16-23 TERM TRANSMITTER
    24-31 TERM RECEIVER
*/
int supportDeviceSemaphores[32];

/*
    Important for a more graceful conclusion of istantiatorProcess.
*/
int masterSemaphore;

/*
    This function inizializes the Phase 3 data structures: 
    -the Swap Pool table and Swap Pool semaphore.
    -semaphores for each sharable I/P device.
    
    It also initializes, launches and then terminates 8 U-procs
*/
void istantiatorProcess();

/*
    Given the device type, device number and a bool that indicates
    if is the terminal receiver, this functions returns a pointer 
    to the semaphore corresponding to that specific device.
*/
int *getSupDevSem(int devType, int devNum, int isReceiver);

#endif