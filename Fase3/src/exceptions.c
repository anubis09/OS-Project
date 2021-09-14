#include "../include/exceptions.h"

void exceptionsHandler()
{
    /*For understanding which type of exception happened, need to get the execCode of the cause register.*/
    unsigned int causeCode = getCAUSE() & GETEXECCODE;
    /*Right-shifting because execCode are bits 2-6, and we want them in bits 0-4 */
    causeCode >>= CAUSESHIFT;
    switch (causeCode)
    {
    case 0:
        /* Interrupt exception, so call Nucleus’s device interrupt handler*/
        interruptHandler();
        break;
    case 1 ... 3:
        /*TLB exception, so call Nucleus’s TLB exception handler*/
        TLB_Handler();
        break;
    case 8:
        /*Syscall exception, so call Nucleus’s SYSCALLexception handler*/
        syscallDispatcher();
        break;
    default:
        /*Case 4-7, 9-12 --> Trap exception, so call Nucleus’s Program Trap exception handler*/
        programTrapHandler();
        break;
    }
}

/*
    Returns the time passed since the process status was loaded, to now.
*/
HIDDEN cpu_t updateTime()
{
    unsigned int thisTime = getTIMER() / (*((cpu_t *)TIMESCALEADDR));
    return (TIMESLICE - thisTime);
}

/*
    This function returns control after a syscall exception.
    If the syscall is a non-blocking syscall ,returns control to the caller.
    If the syscall is a blocking syscall, leaves control to the scheduler.
*/
HIDDEN void retControl(state_t *proc_state, int isBlocking)
{
    /*We need to update pc, otherwise we will enter an infinite syscall loop*/
    proc_state->pc_epc += WORDLEN;
    if (!isBlocking)
    {
        /*Non-blocking syscall*/
        LDST(proc_state);
    }
    else
    {
        /*Blocking syscall, updates the current process processor_status, and the processor time used.*/
        assignStateT(&currentProcess->p_s, proc_state);
        currentProcess->p_time += updateTime();
        scheduler();
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
HIDDEN void create_process(state_t *proc_state)
{
    pcb_PTR newProcess = allocPcb();
    int ret_val = OK;
    if (newProcess == NULL)
    {
        /*No more free pcb to allocate*/
        ret_val = -1;
    }
    else
    {
        /*When SYS1 is called, the second and third input should be a state_t* and a support_t*
        that are needed for the initialization of the pcb.*/
        state_t *statep = (state_t *)proc_state->reg_a1;
        if (statep != NULL)
        {
            support_t *supportp = (support_t *)proc_state->reg_a2;
            assignStateT(&newProcess->p_s, statep);
            newProcess->p_supportStruct = supportp;
            newProcess->p_time = 0;
            newProcess->p_semAdd = NULL;
            insertProcQ(&readyQueue, newProcess);
            insertChild(currentProcess, newProcess);
            /*There is a new process started*/
            processCount++;
        }
        else
        {
            /*State p is null and we do not allocate a new pcb*/
            freePcb(newProcess);
            ret_val = -1;
        }
    }
    proc_state->reg_v0 = ret_val;
    retControl(proc_state, FALSE);
}

/*
    Terminate the whole subtree (process and siblings included) of the process passed as input
*/
HIDDEN void terminate_SubTree(pcb_PTR process)
{
    if (process->p_child != NULL)
    {
        terminate_SubTree(process->p_child);
    }
    if (process->p_next_sib != NULL)
    {
        terminate_SubTree(process->p_next_sib);
    }
    /*Remove the process from the ready queue, if isn't in there, outprocQ returns NULL*/
    outProcQ(&readyQueue, process);
    outChild(process);
    if (process->p_semAdd != NULL && *(process->p_semAdd) < 0)
    {
        /*A valid semaphore*/
        if (process->p_semAdd >= &device_Semaphore[0] &&
            process->p_semAdd <= &device_Semaphore[PSEUDOCLOCKSEM])
        {
            /*Device semaphore*/
            softBlockCount--;
        }
        else
        {
            /*Non device semaphore*/
            (*process->p_semAdd)++;
        }
        /*Remove the process from the semaphore queue*/
        outBlocked(process);
        process->p_semAdd = NULL;
    }
    processCount--;
    freePcb(process);
}

/*
    TerminateProcess (SYS2)
    This services causes the executing process to cease to exist.
    In addition, recursively, all progeny of this process are terminated as well. 
*/
HIDDEN void terminate_process(pcb_PTR process)
{
    outChild(process);
    if (!emptyChild(process))
    {
        terminate_SubTree(process->p_child);
    }
    processCount--;
    freePcb(process);
    /*There is no current process to return control to, so we call the scheduler. */
    scheduler();
}

/*
    Passeren (SYS3)
    This service requests the Nucleus to perform a P operation on a semaphore.
    Depending on the value of the semaphore, control is either returned to the
    Current Process, or this process is blocked on the ASL (transitions from “running”
    to “blocked”) and the Scheduler is called.  
*/
HIDDEN void Passeren(state_t *proc_state)
{
    int *semaddr = (int *)proc_state->reg_a1;
    int block = FALSE;
    if (semaddr != NULL)
    {
        (*semaddr)--;
        if ((*semaddr) < 0)
        {
            int semNotAllocated = insertBlocked(semaddr, currentProcess);
            if (semNotAllocated)
            {
                /*No more free semaphore to allocate.*/
                PANIC();
            }
            else
            {
                block = TRUE;
            }
        }
    }
    retControl(proc_state, block);
}

/*
    Verhogen (V) (SYS4)
    This service requests the Nucleus to perform a V operation on a semaphore.
*/
HIDDEN void Verhogen(state_t *proc_state)
{
    int *semaddr = (int *)proc_state->reg_a1;
    if (semaddr != NULL)
    {
        (*semaddr)++;
        pcb_PTR fp = removeBlocked(semaddr);

        if (fp != NULL)
        {
            fp->p_semAdd = NULL;
            insertProcQ(&readyQueue, fp);
        }
    }
    retControl(proc_state, FALSE);
}

/*
    WAIT_FOR_I/O_DEVICE SYS(5)
    This service performs a P operation on the semaphore that the Nucleus maintains
    for the I/O device indicated by the values in a1,a2, and optionally a3.
*/
HIDDEN void wait_IOdevice(state_t *proc_state)
{
    int interruptLine = proc_state->reg_a1;
    int nSubDevice = proc_state->reg_a2;
    int waitForTermRead = proc_state->reg_a3;
    int *deviceSem = NULL;
    if (interruptLine >= 3 && interruptLine <= 7 && nSubDevice >= 0 && nSubDevice <= 7)
    {
        /*Syscall with the right format*/
        deviceSem = devSem_Access(interruptLine, nSubDevice, waitForTermRead);
        softBlockCount++;
        proc_state->reg_a1 = (int)deviceSem;
        Passeren(proc_state);
    }
    else
    {
        /*Syscall with a wrong format*/
        retControl(proc_state, FALSE);
    }
}

/*
    Get_CPU_Time (SYS6)
    This service requests that the accumulated processor time (in microseconds)
    used by the requesting process be placed/returned in the caller’sv0
*/
HIDDEN void Get_Cpu_Time(state_t *proc_state)
{
    proc_state->reg_v0 = currentProcess->p_time + updateTime();
    retControl(proc_state, FALSE);
}

/*
Wait For Clock (SYS7)
This service performs a P operation on the Nucleus maintained Pseudo-clock
semaphore. This semaphore is V’ed every 100 milliseconds by the Nucleus.
*/
HIDDEN void Wait_For_Clock(state_t *proc_state)
{
    softBlockCount++;
    proc_state->reg_a1 = (int)&device_Semaphore[PSEUDOCLOCKSEM];
    /*We uploaded reg a1 with device semaphore addres, now we are ready to use PASSEREN*/
    Passeren(proc_state);
}

/*  
    GetSUPPORTData (SYS8)
    return the value of p_supportStruct from the current process
*/
HIDDEN void get_support_data(state_t *proc_state)
{
    proc_state->reg_v0 = (int)currentProcess->p_supportStruct;
    retControl(proc_state, FALSE);
}

/*
    For SYSCALL exceptions numbered 9 and above, Program Trap and TLB exceptions,
    the Nucleus will take one of two actions depending on whether the current process 
    was provided a non-NULL value for its Support Structure pointer when it was created:
    1) If the current process's support structureis NULL, then the exception should be handled as a SYS2.
    2) If the Current Process’spsupportStructis non-NULL. The handlingof the exception is “passed up.”
*/
HIDDEN void passUp_Die(int except)
{
    if (currentProcess->p_supportStruct == NULL)
    {
        /*Die portion*/
        terminate_process(currentProcess);
    }
    else
    {
        /*Pass up portion*/
        state_t *proc_state = (state_t *)BIOSDATAPAGE;
        assignStateT(&(currentProcess->p_supportStruct->sup_exceptState[except]), proc_state);
        unsigned int stackPtr, status, pc;
        stackPtr = currentProcess->p_supportStruct->sup_exceptContext[except].c_stackPtr;
        status = currentProcess->p_supportStruct->sup_exceptContext[except].c_status;
        pc = currentProcess->p_supportStruct->sup_exceptContext[except].c_pc;
        LDCXT(stackPtr, status, pc);
    }
}

void syscallDispatcher()
{
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    if (isKernelModeP(proc_state->status))
    {
        /*Process was in kernel mode*/
        int sysType = proc_state->reg_a0;
        switch (sysType)
        {
        case CREATEPROCESS:
            /*Syscall 1*/
            create_process(proc_state);
            break;
        case TERMPROCESS:
            /*Syscall 2*/
            terminate_process(currentProcess);
            break;
        case PASSEREN:
            /*Syscall 3*/
            Passeren(proc_state);
            break;
        case VERHOGEN:
            /*Syscall 4*/
            Verhogen(proc_state);
            break;
        case IOWAIT:
            /*Syscall 5*/
            wait_IOdevice(proc_state);
            break;
        case GETTIME:
            /*Syscall 6*/
            Get_Cpu_Time(proc_state);
            break;
        case CLOCKWAIT:
            /*Syscall 7*/
            Wait_For_Clock(proc_state);
            break;
        case GETSUPPORTPTR:
            /*Syscall 8*/
            get_support_data(proc_state);
            break;
        default:
            /*Syscall 9 or above*/
            passUp_Die(GENERALEXCEPT);
            break;
        }
    }
    else
    {
        /*Process was in user mode, TRAP exception(Reserved Instruction)*/
        setCAUSE(PRIVINSTR << CAUSESHIFT);
        exceptionsHandler();
    }
}

void programTrapHandler()
{
    /*The Nucleus Program Trap exception handler should perform a standard PassUp or Die operation using theGENERALEXCEPTindex value.*/
    passUp_Die(GENERALEXCEPT);
}

void TLB_Handler()
{
    /*The Nucleus TLB exception handler should perform a standard Pass Up orDie operation using thePGFAULTEXCEPTindex value.*/
    passUp_Die(PGFAULTEXCEPT);
}

void uTLB_RefillHandler()
{
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    unsigned int entry_hi = proc_state->entry_hi;
    /*here i am getting the VPN, then i am only taking the last 2 hexa digits
    because they defines the offseet in the pagetable entry.*/

    int pT_Entry = ((entry_hi & GETPAGENO) >> VPNSHIFT) & 0xFF;
    if (pT_Entry > 30) /*check se è lo stack.*/
        pT_Entry = 31;
    pteEntry_t pageTable_Entry = currentProcess->p_supportStruct->sup_privatePgTbl[pT_Entry];
    setENTRYHI(pageTable_Entry.pte_entryHI);
    setENTRYLO(pageTable_Entry.pte_entryLO);
    TLBWR();
    LDST(proc_state);
    /*
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST((state_t *)0x0FFFF000);
    */
}
