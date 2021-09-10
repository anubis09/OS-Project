#include "../include/sysSupport.h"

void programTrap(int asid)
{
    freeME(asid);
    /*TERMINATE()*/
}

HIDDEN void retControl(state_t *proc_state)
{
    proc_state->pc_epc += WORDLEN;
    LDST(proc_state);
}

void supportSyscallDispatcher(support_t *sup_struct)
{
    int sysType = sup_struct->sup_exceptState[GENERALEXCEPT].reg_a0;
    switch (sysType)
    {
    case TERMINATE:
        /* code */
        break;
    case GET_TOD:
        /* code */
        break;
    case WRITEPRINTER:
        /* code */
        break;
    case WRITETERMINAL:
        /* code */
        break;
    case READTERMINAL:
        /* code */
        break;
    default:
        /*syscode > 13*/
        programTrap(sup_struct->sup_asid);
        break;
    }
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
        programTrap(sup_struct->sup_asid);
        break;
    }
}