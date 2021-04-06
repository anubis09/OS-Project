#ifndef INCLUDE_H
#define INCLUDE_H

#include "/usr/include/umps3/umps/libumps.h"
#include "asl.h"
#include "pcb.h"
#include "scheduler.h"
#include "initialize.h"
#include "interrupts.h"
#include "exceptions.h"

/*
    Takes as input a pointer to an unsigned int variable
    that must be initialized with the value of the status register,
    and changes it so that the processor will have Timer disabled.
 */
void disable_timer(unsigned int*);

/*
    Takes as input an unsigned int that must be a value of a status(process or processor),
    returns TRUE if KUp==0, FALSE if it's =1.
*/
int isKernelModeP(unsigned int);

/*
    Function that assign each field of proc_state to toAssign.
*/
void assignStateT(state_t *toAssign, state_t *proc_state);

/*
    Takes as input the device type, device number and TRUE if is terminal receiver FALSE otherwhise,
    and return the pointer to the right semaphore.
*/
int *devSem_Access(int device, int devNo, int isTerminalReceiver);

#endif

