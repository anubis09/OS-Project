#ifndef INITPROC_H
#define INITPROC_H

#include <umps3/umps/libumps.h>
#include "pandos_types.h"
#include "pandos_const.h"
#include "vmSupport.h"

#define DISK 0
#define FLASH 1
#define NETWORK 2
#define PRINTER 3
#define TERMINAL 4
#define SUPDEVSEMNUM 48

/*
    0-7 DISK
    8-15 FLASH
    16-23 NETWORK
    24-31 PRINTER
    32-39 TERM TRANSMITTER
    40-47 TERM RECEIVER
*/
int supportDeviceSemaphores[SUPDEVSEMNUM];

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
    devNum goes from 1 to 8
*/
int *getSupDevSem(int devType, int devNum, int isReceiver);

#endif