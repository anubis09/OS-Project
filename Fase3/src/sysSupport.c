#include "../include/sysSupport.h"

void programTrap(int asid)
{
    freeME(asid);
    /*TERMINATE()*/
}

/*al posto di devNo bisogna mettere asid - 1 e al posto di intLineNo TERMINT per terminale*/
HIDDEN devregtr *getDeviceRegAddr(int intLineNo, int devNo)
{
    return (devregtr *)(DEVREGBASE + (intLineNo - STARTINTLINEDEVICE) * 0x80 + devNo * 0x10);
}

void supportSyscallDispatcher(support_t *sup_struct)
{
    state_t *proc_state = &sup_struct->sup_exceptState[GENERALEXCEPT];
    int asid = sup_struct->sup_asid;
    int sysType = proc_state->reg_a0;
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
    proc_state->pc_epc += WORDLEN;
    LDST(proc_state);
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