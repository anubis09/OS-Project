#include "../include/interrupts.h"
#define BITMAPBASEADDR 0x10000040
#define TERMSTATREAD 0xFF
#define GETIP 0xFF00

/*
    Given an specific Interrupting Devices Bit Map return the number of the device 
    with the greatest priority which has an interrupt pending
*/
HIDDEN int checkdevNo(unsigned int address)
{
    if ((DEV0ON & address) == DEV0ON)
        return 0;
    else if ((DEV1ON & address) == DEV1ON)
        return 1;
    else if ((DEV2ON & address) == DEV2ON)
        return 2;
    else if ((DEV3ON & address) == DEV3ON)
        return 3;
    else if ((DEV4ON & address) == DEV4ON)
        return 4;
    else if ((DEV5ON & address) == DEV5ON)
        return 5;
    else if ((DEV6ON & address) == DEV6ON)
        return 6;
    else
        return 7;
}

/* 
    Returns the device register address associated with the given interrupt line and device number 
*/
HIDDEN devregtr *getDeviceRegAddr(int intLineNo, int devNo)
{
    return (devregtr *)(DEVREGBASE + (intLineNo - STARTINTLINEDEVICE) * 0x80 + devNo * 0x10);
}

/* 
    Function that returns TRUE if the terminal device is in a status of writing or FALSE if is on one of reading 
*/
HIDDEN int statusTerm(devregtr *devReg)
{
    if ((*(devReg + 2) & TERMSTATREAD) == OKCHARTRANS)
        return TRUE;
    else
        return FALSE;
}

/* 
    Acknowledge the outstanding interrupt. This is accomplished by writing the acknowledge 
    command code in the interrupting device’s device register 
*/
HIDDEN void acknowledge(int interrupt, devregtr *devReg)
{
    devregtr *ack = NULL;
    if (interrupt == TERMINT && statusTerm(devReg))
    {
        ack = (devReg + 3);
    }
    else
        ack = (devReg + 1);

    *ack = ACK;
}

/*
    Returns control to the scheduler if the processor was in WAIT state, 
    otherwhise returns control to the current process.
*/
HIDDEN void ret_Control(state_t *proc_state)
{
    if ((proc_state->status & TEBITON) == 0 && (proc_state->status & IEPON) == IEPON &&
        (proc_state->status & IMON) == IMON)
    {
        /*The saved process was in WAIT state, so we return control to the scheduler*/
        scheduler();
    }
    else
    {
        LDST(proc_state);
    }
}

/* 
    Perform a V operation on the Nucleus maintained semaphore associated with the (sub)device. 
    This operation should unblock the process which initiated this I/O operation
*/
HIDDEN pcb_PTR vOperation(int *semaddr)
{
    (*semaddr)++;
    pcb_PTR unblocked_proc = removeBlocked(semaddr);
    if (unblocked_proc != NULL)
    {
        unblocked_proc->p_semAdd = NULL;
        insertProcQ(&readyQueue, unblocked_proc);
        softBlockCount--;
    }
    return unblocked_proc;
}

/*
    Handler of device interrupts
*/
HIDDEN void externalDeviceint(unsigned int cause_IP)
{
    int *idbmA; /*Bitmap pointer*/
    int ioStatus, deviceInt, deviceNo, *semaddr = NULL;
    int isReceiver = FALSE;
    devregtr *devRegAddr;

    /* Determine which device with an outstanding interrupt has the highest priority */
    if ((cause_IP & DISKINTERRUPT) == DISKINTERRUPT)
    {
        idbmA = (int *)BITMAPBASEADDR;
        deviceInt = DISKINT;
    }
    else if ((cause_IP & FLASHINTERRUPT) == FLASHINTERRUPT)
    {
        idbmA = (int *)(BITMAPBASEADDR + 0x04);
        deviceInt = FLASHINT;
    }
    else if ((cause_IP & NETWORKINTERRUPT) == NETWORKINTERRUPT)
    {
        idbmA = (int *)(BITMAPBASEADDR + 0x08);
        deviceInt = NETWINT;
    }
    else if ((cause_IP & PRINTINTERRUPT) == PRINTINTERRUPT)
    {
        idbmA = (int *)(BITMAPBASEADDR + 0x0C);
        deviceInt = PRNTINT;
    }
    else if ((cause_IP & TERMINTERRUPT) == TERMINTERRUPT)
    {
        idbmA = (int *)(BITMAPBASEADDR + 0x10);
        deviceInt = TERMINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);

        if (statusTerm(devRegAddr))
            ioStatus = *(devRegAddr + 2);
        else
        {
            /*Receiver */
            isReceiver = TRUE;
            ioStatus = *devRegAddr;
        }
    }
    else
    {
        /*Uncatched interrupt*/
        PANIC();
    }
    if (deviceInt != TERMINT)
    {
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
        ioStatus = *devRegAddr;
    }
    acknowledge(deviceInt, devRegAddr);
    semaddr = devSem_Access(deviceInt, deviceNo, isReceiver);
    pcb_PTR unblocked_proc = vOperation(semaddr);
    if (unblocked_proc != NULL)
    {
        /*Place the newly unblocked pcb in the readyqueue*/
        unblocked_proc->p_s.reg_v0 = ioStatus;
    }
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    ret_Control(proc_state);
}

/*
    Handler of timer interrups
*/
HIDDEN void timerint(unsigned int cause_IP)
{
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    if (cause_IP == LOCALTIMERINT)
    {
        updatePLT; /*Ack interrupt*/
        assignStateT(&currentProcess->p_s, proc_state);
        currentProcess->p_time += TIMESLICE;
        insertProcQ(&readyQueue, currentProcess);
        scheduler();
    }
    else
    {
        /*Perform a V operation on the Nucleus maintained semaphore associated with the pseudo-clock. */
        LDIT(PSECOND); /*Acknowledge the interrupt.*/
        pcb_PTR unblocked_process;
        int *clockSem = &device_Semaphore[PSEUDOCLOCKSEM];
        /*Should unblock all pcbs on the semaphore.*/
        do
        {
            unblocked_process = vOperation(clockSem);
        } while (unblocked_process != NULL);
        *clockSem = 0;
        ret_Control(proc_state);
    }
}

void interruptHandler()
{
    /*For understanding which interrupt lines have pending interrupts*/
    unsigned int causeCode = getCAUSE() & GETIP;

    if ((causeCode & LOCALTIMERINT) == LOCALTIMERINT)
    {
        timerint(LOCALTIMERINT);
    }
    else if ((causeCode & TIMERINTERRUPT) == TIMERINTERRUPT)
    {
        timerint(TIMERINTERRUPT);
    }
    else if ((causeCode & INTERPROCINT) == INTERPROCINT)
    {
        PANIC();
    }
    else
    {
        externalDeviceint(causeCode);
    }
}
