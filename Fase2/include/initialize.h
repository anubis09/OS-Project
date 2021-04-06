#ifndef INITIALIZE_H
#define INITIALIZE_H

#include "../include/include.h"
#define SEMAPHORENUM 49
#define PSEUDOCLOCKSEM 48

/*Integer indicating the number of started, but not yet terminated processes.*/
int processCount;

/*This integer is the numberof started, but not terminated processes that in are the “blocked” statedue to an I/O or timer request.*/
int softBlockCount;

/* Tail pointer to a queue ofpcbs that are in the “ready” state.*/
pcb_PTR readyQueue;

/*Pointer to thepcbthat is in the “running” state, i.e.the current executing process.*/
pcb_PTR currentProcess;

/*
    0-7 disk sem, 8-15 flash sem, 16-23 network sem, 24-31 printer sem,
    32-39 terminal transmitter semaphore, 40-47 terminal receiver sem,
    48 pseudo clock sem 
*/
int device_Semaphore[SEMAPHORENUM];

#endif