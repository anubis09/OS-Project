#include "interrupts.h"

/*
    given an specific Interrupting Devices Bit Map return the number of the device with the greatest priority which has an interrupt pending
*/
HIDDEN int checkdevNo(unsigned int address){
    if((DEV0ON & address) == DEV0ON)
        return 0;
    else if((DEV1ON & address) == DEV1ON)
        return 1;
    else if((DEV2ON & address) == DEV2ON)
        return 2;
    else if((DEV3ON & address) == DEV3ON)
        return 3;
    else if((DEV4ON & address) == DEV4ON)
        return 4;
    else if((DEV5ON & address) == DEV5ON)
        return 5;
    else if((DEV6ON & address) == DEV6ON)
        return 6;   
    else
        return 7;
}

/* 
    returns the device register address associated with the given interrupt line and device number 
*/
HIDDEN devregtr *getDeviceRegAddr(int intLineNo, int devNo){
    return (devregtr *)(DEVREGBASE + (intLineNo-STARTINTLINEDEVICE)*0x80 + devNo*0x10);
}

/* 
    function that returns TRUE if the terminal device is in a status of writing or FALSE if is on one of reading 
*/
HIDDEN int statusTerm(devregtr *devReg){
    if((*(devReg + 2) & 0x00FF) == 5)
        return TRUE;
    else
        return FALSE;
}

/* 
    Acknowledge the outstanding interrupt.  This is accomplished by writing the acknowledge command code in the interrupting device’s device register 
*/
HIDDEN void acknowledge(int interrupt, devregtr *devReg ){
    devregtr *ack = NULL;
    if(interrupt == TERMINT){
        if(statusTerm(devReg)) ack = (devregtr *) (devReg + 3);
        else ack = (devregtr *) (devReg + 1);
    } 
    else ack = (devregtr *) (devReg + 1); 
    
    *ack = ACK;
}

/* 
    check to see if the processor is the process is in a state of WAIT 
*/
HIDDEN int checkWait(state_t *proc_state){
    if((proc_state->status & TEBITON) == 0 && 
    (proc_state->status & IEPON) == IEPON &&
    (proc_state->status & IMON) == IMON){
        return TRUE;
    }
    else{
        return FALSE;
    }
}

/* 
    Perform a V operation on the Nucleus maintained semaphore associated with the (sub)device. 
    This operation should unblock the process which initiated this I/O operation
*/
HIDDEN pcb_PTR VDevice(int *semaddr){
    (*semaddr)++;
    pcb_PTR fp = removeBlocked(semaddr);
    /*if fp == NULL then an ancestor of fp terminanted.*/
    return fp;
}

/*
    handler of non-timer interrups
*/
HIDDEN void externalDeviceint(unsigned int cause_IP){
    int *idbmA;
    int ioStatus, deviceInt, deviceNo, *semaddr = NULL;
    int isReceiver = 0;
    devregtr *devRegAddr;

    /* determine which device with an outstanding interrupt is the highest priority */
    if((cause_IP & DISKINTERRUPT) == DISKINTERRUPT){
        idbmA = (int *)0x10000040;
        deviceInt = DISKINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
    }
    else if((cause_IP & FLASHINTERRUPT) == FLASHINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x04);
        deviceInt = FLASHINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
    }
    else if((cause_IP & NETWORKINTERRUPT) == NETWORKINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x08);
        deviceInt = NETWINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
    }
    else if((cause_IP & PRINTINTERRUPT) == PRINTINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x0C);
        deviceInt = PRNTINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
    }
    else if((cause_IP & TERMINTERRUPT) == TERMINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x10);
        deviceInt = TERMINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);

        if(statusTerm(devRegAddr))
            ioStatus = *(devRegAddr + 2);
        else {
            /*receiver */
            isReceiver = 1;
            ioStatus = *devRegAddr;
        }
    }
    else{
        PANIC();
    }
    if(deviceInt != TERMINT) ioStatus = *devRegAddr;

    acknowledge(deviceInt, devRegAddr);

    semaddr = &device_Semaphore[(deviceInt-3+isReceiver)*8 + deviceNo];
    pcb_PTR unblocked_proc = VDevice(semaddr);
    if(unblocked_proc != NULL){
        /*place the newly unblocked pcb in the readyqueue*/
        unblocked_proc->p_semAdd = NULL;
        unblocked_proc->p_s.reg_v0 = ioStatus;
        insertProcQ(&readyQueue, unblocked_proc);
        softBlockCount--;
    }
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    
    if(checkWait(proc_state)){
        /*goes back to the scheduler*/
        dispatch();
    }
    else{
        /*return control to the current process*/
        LDST(proc_state);
    }
}

/*
    Perform a V operation on the Nucleus maintained semaphore associated with the pseudo-clock. 
    Unblock ALL pcbs associated with this semaphore.
*/
HIDDEN void VClock(){ 
    int *semaddr = &device_Semaphore[PSEUDOCLOCKSEM];
    pcb_PTR blocked_process = removeBlocked(semaddr);
    
    while(blocked_process != NULL){
        blocked_process->p_semAdd = NULL;
        insertProcQ(&readyQueue, blocked_process);
        softBlockCount--;
        blocked_process = removeBlocked(semaddr);
    }
    device_Semaphore[PSEUDOCLOCKSEM] = 0;
}

/*
    handler of timer interrups
*/
HIDDEN void timerint(unsigned int cause_IP){
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    if(cause_IP == LOCALTIMERINT){
        updatePLT;
        assignStateT(&currentProcess->p_s, proc_state);
        currentProcess->p_time += TIMESLICE;
        insertProcQ(&readyQueue,currentProcess);
        dispatch();
    } else if (cause_IP == TIMERINTERRUPT){
        LDIT(PSECOND);
        /*should unblock all pcbs on the semaphore.*/
        VClock();
        if(checkWait(proc_state)){
            /*we were in a wait state*/
            dispatch();
        }
        else{
            LDST(proc_state);
        }
    }
}

void interruptHandler(){  
    /*for understanding which interrupt lines have pending interrupts*/  
    unsigned int causeCode = 0xFF00 & getCAUSE();

    if((causeCode & LOCALTIMERINT) == LOCALTIMERINT){
        timerint(causeCode & LOCALTIMERINT);
    } 
    else if((causeCode & TIMERINTERRUPT) == TIMERINTERRUPT) {
        timerint(causeCode & TIMERINTERRUPT);
    }
    else {
        externalDeviceint(causeCode);
    }
}

