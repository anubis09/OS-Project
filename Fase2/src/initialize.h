#ifndef INITIALIZE_H
#define INITIALIZE_H

#include "../include/include.h"
#include "scheduler.h"
#include "exceptions.h"

int processCount;
int softBlockCount;
pcb_PTR readyQueue;
pcb_PTR currentProcess;

int pseudoClock_sem;
int disk_sem[8];
int flash_sem[8];
int network_sem[8];
int printer_sem[8];
/*terminal devices semaphore*/
int transmitter_sem[8];
int receiver_sem[8];


#endif