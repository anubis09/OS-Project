#include "exceptions.h"

void exceptionshandler(){
    unsigned int causeCode = getCAUSE() & GETEXECCODE;
    causeCode = causeCode >> CAUSESHIFT;
    switch (causeCode){
    case 0:
        /*call Nucleus’s device interrupt handler*/
        break;
    case 1 ... 3 :
        /*Nucleus’s TLB exception handler*/
        passUp_Die(PGFAULTEXCEPT);
        break;
    case 8:
        /*Nucleus’sSYSCALLexception handler*/
        syscallDispatcher();
        break;
    default:
        /*case 4-7, 9-12 -->  Nucleus’s Program Trap exception handler*/
        passUp_Die(GENERALEXCEPT);
        break;
    }
}

void passUp_Die(int except){
    if(currentProcess->p_supportStruct == NULL){
        /*die portion*/
        /*call syscall 2*/
    }
    else{
        /*pass up portion*/
        state_t *proc_state = (state_t *)BIOSDATAPAGE;
        currentProcess->p_supportStruct->sup_exceptState[except] = *proc_state;
        unsigned int stackPtr,status,pc;
        stackPtr = currentProcess->p_supportStruct->sup_exceptContext[except].c_stackPtr;
        status = currentProcess->p_supportStruct->sup_exceptContext[except].c_status;
        pc = currentProcess->p_supportStruct->sup_exceptContext[except].c_pc;
        LDCXT(stackPtr,status,pc);
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
        newProcess->p_time = 0;
        newProcess->p_semAdd = NULL;
        insertProcQ(&readyQueue, newProcess);
        insertChild(currentProcess, newProcess);
        processCount++;
        ret_val = 0;
    }
    proc_state->reg_v0 = ret_val;
}

HIDDEN void retControl(state_t *proc_state, int isBlocking){
    /*we need to update pc, otherwise we will enter an infinite syscall loop*/
    proc_state->pc_epc += WORDLEN;
    if(!isBlocking){
        /*non-blocking syscall*/
        LDST(proc_state);
    }
    else{
        /*blocking syscall*/
        currentProcess->p_s = *proc_state;
        /*update cpu time*/
        /*softBlockCount++;??*/
        dispatch();
    }   
}

void syscallDispatcher(){
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    if(isKernelModeP(proc_state->status)){
        /*process was in kernel mode*/
        int sysType = proc_state->reg_a0;
        switch (sysType)
        {
        case CREATEPROCESS:
            create_process(proc_state);
            retControl(proc_state, FALSE);
            break;
        case TERMPROCESS:
            /*syscall 2*/
            dispatch();
            break;
        case PASSEREN:
            /*syscall 3*/
            retControl(proc_state, TRUE);
            break;
        case VERHOGEN:
            /*syscall 4*/
            retControl(proc_state, FALSE);
            break;
        case IOWAIT:
            /*syscall 5*/
            retControl(proc_state, TRUE);
            break;
        case GETTIME:
            /*syscall 6*/
            retControl(proc_state, FALSE);
            break;
        case CLOCKWAIT:
            /*syscall 7*/
            retControl(proc_state, TRUE);
            break;
        case GETSUPPORTPTR:
            /*syscall 8*/
            retControl(proc_state, FALSE);
            break;
        default:
            /*syscall 9 or above*/
            passUp_Die(GENERALEXCEPT);
            break;
        }
    }
    else{
        /*process was in user mode*/
        setCAUSE(PRIVINSTR<<CAUSESHIFT);
        exceptionshandler();
    }
}
