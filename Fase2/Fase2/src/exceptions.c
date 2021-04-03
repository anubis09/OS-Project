#include "exceptions.h"

void exceptionshandler(){
    /*for understanding which type of exception happened, need to get the execCode of the cause register.*/
    unsigned int causeCode = getCAUSE() & GETEXECCODE;
    /*right-shifting because execCode are bits 2-6, and we want them in bits 0-4 */
    causeCode >>= CAUSESHIFT;
    switch (causeCode){
    case 0:
        /* Interrupt exception, so call Nucleus’s device interrupt handler*/
        interruptHandler();
        break;
    case 1 ... 3 :
        /*TLB exception, so call Nucleus’s TLB exception handler*/
        passUp_Die(PGFAULTEXCEPT);
        break;
    case 8:
        /*Syscall exception, so call Nucleus’s SYSCALLexception handler*/
        syscallDispatcher();
        break;
    default:
        /*case 4-7, 9-12 --> Trap exception, so call Nucleus’s Program Trap exception handler*/
        passUp_Die(GENERALEXCEPT);
        break;
    }
}

/*
    return the time passed since the process status was loaded, to now.
*/
HIDDEN cpu_t updateTime(){
    signed int thisTime;
    STCK(thisTime);
    return (thisTime - timeStart); 
}

/*
    This function returns control after a syscall exception.
    If the syscall is a non-blocking syscall ,returns control to the caller.
    If the syscall is a blocking syscall, leaves control to the scheduler.
*/
HIDDEN void retControl(state_t *proc_state, int isBlocking){
    /*we need to update pc, otherwise we will enter an infinite syscall loop*/
    proc_state->pc_epc += WORDLEN;
    if(isBlocking == FALSE){
        /*non-blocking syscall*/
        LDST(proc_state);
    }
    else{
        /*blocking syscall, updates the current process processor_status, and the processor time used.*/
        assegnamentoStruct(currentProcess,proc_state);
        currentProcess->p_time += updateTime();
        dispatch();
    }   
}

/*
    CreateProcess (SYS1)
    When requested, this service causes a new process, said to be a progeny of the caller, to be created.
    Takes as input the saved status of the process and, if possible, creates, initializes, a new pcb,
    and inserts it in the readyQueue and in the child list of the current process.
    If the new process cannot be created due to lack of resources, an error code of -1 is placed/returned 
    in the caller’sv0, otherwise, returnthe value 0 in the caller’sv0.
*/
HIDDEN void create_process(state_t *proc_state){
    pcb_PTR newProcess = allocPcb(); 
    int ret_val = 0;
    if(newProcess == NULL){
        /*no more free pcb to allocate*/
        ret_val = -1;
    }
    else{
        /*when SYS1 is called, the second and third input should be a state_t* and a support_t*
        that are needed for the initialization of the pcb. */
        state_t *statep = (state_t *)proc_state->reg_a1;
        support_t *supportp = (support_t*)proc_state->reg_a2;
        newProcess->p_s.status = statep->status;
        newProcess->p_supportStruct = supportp;
        newProcess->p_time = 0;
        newProcess->p_semAdd = NULL;
        insertProcQ(&readyQueue, newProcess);
        insertChild(currentProcess, newProcess);
        /*there is a new process started*/
        processCount++;
    }
    proc_state->reg_v0 = ret_val;
    retControl(proc_state, FALSE);
}

/*
    TerminateProcess (SYS2)
    This services causes the executing process to cease to exist.
    In ad-dition, recursively, all progeny of this process are terminated as well. 
*/
HIDDEN void terminate_process(pcb_PTR process){    

    if(process->p_child != NULL){
        terminate_process(process->p_child);            
    }

    if(process->p_next_sib != NULL){
        terminate_process(process->p_next_sib);
    }
            
    outProcQ(&readyQueue ,process);
    outChild(process);
    freePcb(process);
    processCount--;
    /*serve ancora un controllo se il semaforo è un device semaforo oppure no.*/
    if(process->p_semAdd != NULL && *process->p_semAdd < 0){
        process->p_semAdd++;
        outBlocked(process);
    }
}

/*
    Passeren (SYS3)
    This service requests the Nucleus to perform a P operation on a semaphore.
    Depending on the value of the semaphore, control is either returned to the
    Current Process, or this process is blocked on the ASL (transitions from “running”
    to “blocked”) and the Scheduler is called.  
*/
HIDDEN void Passeren(state_t *proc_state){
    int *semaddr = (int *)proc_state->reg_a1;
	(*semaddr)--;
    int block = FALSE;

	if((*semaddr) < 0){
        int semNotAllocated = insertBlocked(semaddr, currentProcess);
		if(semNotAllocated){
            PANIC();
        }
        else{
            block = TRUE;
        }

	}
    retControl(proc_state,block);
}

/*
Verhogen (V) (SYS4)
This service requests the Nucleus to perform a V operation on a semaphore.
*/
void Verhogen(state_t *proc_state){
    int *semaddr = (int *)proc_state->reg_a1;
	*semaddr += 1;
    pcb_PTR fp = removeBlocked(semaddr);

    if(fp != NULL){
        fp->p_semAdd = NULL;
        insertProcQ(&readyQueue, fp);
    }

    retControl(proc_state,FALSE); 
}

/*
    WAIT_FOR_I/O_DEVICE SYS(5)
*/
HIDDEN void wait_IOdevice(state_t *proc_state){
    int interruptLine = proc_state->reg_a1;
    int nSubDevice = proc_state->reg_a2;
    int waitForTermRead = proc_state->reg_a3;
    int *deviceSem = NULL;
    if(nSubDevice >=0 && nSubDevice <=7){
        switch(interruptLine){
            case 3:
                deviceSem = &disk_sem[nSubDevice];
                break;
            case 4:
                deviceSem = &flash_sem[nSubDevice];
                break;
            case 5:
                deviceSem = &network_sem[nSubDevice];
                break;
            case 6:
                deviceSem = &printer_sem[nSubDevice];
                break;
            case 7:
                if(waitForTermRead == TRUE) deviceSem = &(receiver_sem[nSubDevice]);
                else if(waitForTermRead == FALSE) deviceSem = &(transmitter_sem[nSubDevice]);
                break;
            default:
                break;
        }
        if(deviceSem != NULL){
            softBlockCount++;
            proc_state->reg_a1 = (int)deviceSem;
            Passeren(proc_state);
        }
    }
    else{
        /*syscall with a wrong format*/
        retControl(proc_state,FALSE);
    }

}

/*
    Get_CPU_Time (SYS6)
    This service requests that the accumulated processor time (in microseconds)
    used by the requesting process be placed/returned in the caller’sv0
*/
HIDDEN void Get_Cpu_Time(state_t *proc_state){
    proc_state->reg_v0 = currentProcess->p_time + updateTime();
    retControl(proc_state, FALSE);
}

/*
Wait For Clock (SYS7)
This service performs a P operation on the Nucleus maintained Pseudo-clock
semaphore. This semaphore is V’ed every 100 milliseconds by the Nucleus.
*/
HIDDEN void Wait_For_Clock(state_t *proc_state){

    softBlockCount++;
    proc_state->reg_a1 = (int)&pseudoClock_sem;
    Passeren(proc_state);
}

/*  
    GetSUPPORTData (SYS8)
    return the value of p_supportStruct from the current process
*/
HIDDEN void get_support_data(state_t *proc_state){
    proc_state->reg_v0 = (int)currentProcess->p_supportStruct;
    retControl(proc_state,FALSE);
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
            break;
        case TERMPROCESS:
            /*syscall 2*/
            terminate_process(currentProcess);
            dispatch();
            break;
        case PASSEREN:
            /*syscall 3*/
            Passeren(proc_state);
            break;
        case VERHOGEN:
            /*syscall 4*/
            Verhogen(proc_state);
            break;
        case IOWAIT:
            /*syscall 5*/
            wait_IOdevice(proc_state);
            break;
        case GETTIME:
            /*syscall 6*/
            Get_Cpu_Time(proc_state);
            break;
        case CLOCKWAIT:
            /*syscall 7*/
            Wait_For_Clock(proc_state);
            break;
        case GETSUPPORTPTR:
            /*syscall 8*/
            get_support_data(proc_state);
            break;
        default:
            /*syscall 9 or above*/
            passUp_Die(GENERALEXCEPT);
            break;
        }
    }
    else{
        /*process was in user mode, TRAP exception(Reserved Instruction)*/
        setCAUSE(PRIVINSTR<<CAUSESHIFT);
        exceptionshandler();
    }
}

void passUp_Die(int except){
    if(currentProcess->p_supportStruct == NULL){
        /*die portion*/
        terminate_process(currentProcess);
        dispatch();
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
