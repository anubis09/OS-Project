#ifndef INITIALIZE_H
#define INITIALIZE_H

#include "../include/include.h"
#include "scheduler.h"
#include "exceptions.h"

/*Integer indicating the number of started, but not yet terminated processes.*/
int processCount;

/*This integer is the numberof started, but not terminated processes that in are the “blocked” statedue to an I/O or timer request.*/
int softBlockCount;

/* Tail pointer to a queue ofpcbs that are in the “ready”state.*/
pcb_PTR readyQueue;

/*Pointer to thepcbthat is in the “running” state, i.e.the current executing process.*/
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