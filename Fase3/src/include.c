#include "../include/include.h"
#define STARTINTLINEDEVICE 3

void disable_timer(unsigned int *status)
{
    /*We only need to set to 0 the 27th bit.*/
    *status &= 0b11110111111111111111111111111111;
}

int isKernelModeP(unsigned int status)
{
    unsigned int checkKernelMode = 0b00000000000000000000000000001000;
    checkKernelMode &= status;
    /*With AND bitwise, we will only check the fourth bit, because is the only bit in checkKernelModeP set to 1.
    If checkKernelMode == 0 then the fourth bit in status is 0, and the processor is in kernel mode.
    Otherwise it's 1 and the processor is in user mode.*/
    if (checkKernelMode == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void assignStateT(state_t *toAssign, state_t *proc_state)
{
    toAssign->cause = proc_state->cause;
    toAssign->entry_hi = proc_state->entry_hi;
    for (int i = 0; i < STATE_GPR_LEN; i++)
    {
        toAssign->gpr[i] = proc_state->gpr[i];
    }
    toAssign->hi = proc_state->hi;
    toAssign->lo = proc_state->lo;
    toAssign->pc_epc = proc_state->pc_epc;
    toAssign->status = proc_state->status;
}

int *devSem_Access(int device, int devNo, int isTerminalReceiver)
{
    return &device_Semaphore[(device - STARTINTLINEDEVICE + isTerminalReceiver) * 8 + devNo];
}
