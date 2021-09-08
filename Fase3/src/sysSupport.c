#include "../include/sysSupport.h"

void supportSyscallDispatcher(support_t *sup_struct)
{
}

void programTrap()
{
    /*TERMINATE()*/
    /*non dobbiamo mettere a posto i semafori perchè ci pensa già
    la syscall 2*/
}

void generalExceptionHandler()
{
    support_t *sup_struct = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    unsigned int cause = (sup_struct->sup_exceptState[GENERALEXCEPT].cause & GETEXECCODE) >> CAUSESHIFT;
    switch (cause)
    {
    case SYSEXCEPTION:
        supportSyscallDispatcher(sup_struct);
        break;
    default:
        programTrap(sup_struct);
        break;
    }
}