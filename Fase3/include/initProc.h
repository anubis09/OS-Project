#ifndef INITPROC_H
#define INITPROC_H

#include "include.h"
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
int masterSemaphore;

void istantiatorProcess();
int *getSupDevSem(int devType, int devNum, int isReceiver);

#endif