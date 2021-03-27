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
            create_process(proc_state);
            retControl(proc_state, sysType);
            break;
        case TERMPROCESS:
            /*syscall 2*/
            dispatch();
            break;
        case PASSEREN:
            /*syscall 3*/
            retControl(proc_state, sysType);
            break;
        case VERHOGEN:
            /*syscall 4*/
            retControl(proc_state, sysType);
            break;
        case IOWAIT:
            /*syscall 5*/
            retControl(proc_state, sysType);
            break;
        case GETTIME:
            /*syscall 6*/
            retControl(proc_state, sysType);
            break;
        case CLOCKWAIT:
            /*syscall 7*/
            retControl(proc_state, sysType);
            break;
        case GETSUPPORTPTR:
            /*syscall 8*/
            retControl(proc_state, sysType);
            break;
        default:
            /*syscall 9 or above*/
            /*GENERALEXCEPT*/
            /*Pass Up or Die*/
            break;
        }
    }
    else{
        /*process was in user mode*/
        setCAUSE(PRIVINSTR<<CAUSESHIFT);
        /*call prgogram trap handler*/
    }
}

HIDDEN void create_process(state_t *proc_state){
    pcb_PTR newProcess = allocPcb(); 
    int ret_val;
    if(newProcess == NULL) ret_val = -1;
    else{
        state_t *statep = (state_t *)proc_state->reg_a1;
        support_t *supportp = (support_t*)proc_state->reg_a2;
        newProcess->p_s.status = statep->status;
        newProcess->p_supportStruct = supportp;
        insertProcQ(&readyQueue, newProcess);
        insertChild(currentProcess, newProcess);
        newProcess->p_time = 0;
        newProcess->p_semAdd = NULL;
        ret_val = 0;
    }
    proc_state->reg_v0 = ret_val;
}

HIDDEN void retControl(state_t *proc_state, int sysType){
    /*we need to update pc, otherwise we will enter an infinite syscall loop*/
    proc_state->pc_epc += WORDLEN;
    if(sysType == 1 ||sysType == 4 || sysType == 6 || sysType == 8){
        /*non-blocking syscall*/
        LDST(proc_state);
    }
    else{
        /*blocking sys call*/
        /*currentProcess->p_s = *proc_state;*/
        /*update cpu time*/
        dispatch();
    }   
}
