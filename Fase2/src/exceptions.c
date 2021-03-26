#include "exceptions.h"

void exceptionshandler(){
    unsigned int causeCode = getCAUSE() & GETEXECCODE;
    causeCode = causeCode >> CAUSESHIFT;
    switch (causeCode){
    case 0:
        /*call Nucleus’s device interrupt handler*/
        break;
    case 1 ... 3 :
        /*call Nucleus’s TLB exception handler*/
        break;
    case 8:
        /*call Nucleus’s SYSCALL exception handler*/
        break;
    default:
        /*case 4-7, 9-12*/
        /*call Nucleus’s Program Trap exception handler*/
        break;
    }
}

void syscallDispatcher(){
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    if(isKernelModeP(proc_state) == TRUE){
        /*process was in kernel mode*/
        int sysType = proc_state->reg_a0;
        switch (sysType)
        {
        case CREATEPROCESS:
            /* syscall 1*/
            break;
        case TERMPROCESS:
            /*syscall 2*/
            break;
        case PASSEREN:
            /*syscall 3*/
            break;
        case VERHOGEN:
            /*syscall 4*/
            break;
        case IOWAIT:
            /*syscall 5*/
            break;
        case GETTIME:
            /*syscall 6*/
            break;
        case CLOCKWAIT:
            /*syscall 7*/
            break;
        default:
            /*GETSUPPORTPTR*/
            break;
        }
    }
    else{
        /*process was in user mode*/
        setCAUSE(PRIVINSTR<<CAUSESHIFT);
        /*call prgogram trap handler*/
    }
}
