#include "interrupts.h"
#define NETWORKINTERRUPT 0x00002000
#define DEVREGBASE 0x10000054
#define STARTINTLINEDEVICE 3
typedef unsigned int devregtr;

HIDDEN int checkdevNo(unsigned int check){
    if((DEV0ON & check) == DEV0ON)
        return 0;
    else if((DEV1ON & check) == DEV1ON)
        return 1;
    else if((DEV2ON & check) == DEV2ON)
        return 2;
    else if((DEV3ON & check) == DEV3ON)
        return 3;
    else if((DEV4ON & check) == DEV4ON)
        return 4;
    else if((DEV5ON & check) == DEV5ON)
        return 5;
    else if((DEV6ON & check) == DEV6ON)
        return 6;   
    else
        /*((DEV7ON & check) == 0b10000000)*/
        return 7;
}

HIDDEN devregtr *getDeviceRegAddr(int intLineNo, int devNo){
    return (devregtr *)(DEVREGBASE + (intLineNo-STARTINTLINEDEVICE)*0x80 + devNo*0x10);
}

HIDDEN int transmTerm(int idbmA, devregtr *devReg){
    if((*(devReg + 2) & 0x00FF) == 5){
        return TRUE;
    }
    else{
        return FALSE;
    }
}


HIDDEN void acknowledge(int interrupt, int idbmA, devregtr *devReg ){
    devregtr *ack = NULL;
    if(interrupt == TERMINT){
        if(transmTerm(idbmA, devReg)) ack = (devregtr *) (devReg + 3);
        else ack = (devregtr *) (devReg + 1);
    } 
    else ack = (devregtr *) (devReg + 1); 
    
    *ack = ACK;
}

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

HIDDEN pcb_PTR VDevice(int *semaddr){
    pcb_PTR fp = removeBlocked(semaddr);
    *semaddr += 1;
    /*if fp == NULL then an ancestor of fp terminanted.*/
    return fp;
}

HIDDEN void externalDeviceint(unsigned int IP){
    int *idbmA;
    int ioStatus, deviceInt, deviceNo, *semaddr = NULL;
    devregtr *devRegAddr;
    if((IP & DISKINTERRUPT) == DISKINTERRUPT){
        idbmA = (int *)0x10000040;
        deviceInt = DISKINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
        semaddr = &disk_sem[checkdevNo(*idbmA)]; 
    }
    else if((IP & FLASHINTERRUPT) == FLASHINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x04);
        deviceInt = FLASHINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
        semaddr = &flash_sem[checkdevNo(*idbmA)];
    }
    else if((IP & NETWORKINTERRUPT) == NETWORKINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x08);
        deviceInt = NETWINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
        semaddr = &network_sem[checkdevNo(*idbmA)];
    }
    else if((IP & PRINTINTERRUPT) == PRINTINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x0C);
        deviceInt = PRNTINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);
        semaddr = &printer_sem[checkdevNo(*idbmA)];
    }
    else if((IP & TERMINTERRUPT) == TERMINTERRUPT){
        idbmA = (int *)(0x10000040 + 0x10);
        deviceInt = TERMINT;
        deviceNo = checkdevNo(*idbmA);
        devRegAddr = getDeviceRegAddr(deviceInt, deviceNo);

        if(transmTerm(*idbmA, devRegAddr)){
            semaddr = &transmitter_sem[deviceNo];
            ioStatus = *(devRegAddr + 2);
        } 
        else {
            /*receiver */
            semaddr = &receiver_sem[deviceNo];
            ioStatus = *devRegAddr;
        }
    }
    else{
        PANIC();
    }
    if(deviceInt != TERMINT) ioStatus = *devRegAddr;
    acknowledge(deviceInt, *idbmA, devRegAddr);
    pcb_PTR unblocked_proc = VDevice(semaddr);
    if(unblocked_proc != NULL){
        unblocked_proc->p_semAdd = NULL;
        unblocked_proc->p_s.reg_v0 = ioStatus;
        insertProcQ(&readyQueue, unblocked_proc);
        softBlockCount--;
    }
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    if(checkWait(proc_state)){
        dispatch();
    }
    else{
        LDST(proc_state);
    }
}

HIDDEN void VClock(){ 
    int *semaddr = &pseudoClock_sem;
    pcb_PTR blocked_process = removeBlocked(semaddr);
    
    while(blocked_process!=NULL){
        blocked_process->p_semAdd = NULL;
        insertProcQ(&readyQueue, blocked_process);
        softBlockCount--;
        blocked_process = removeBlocked(semaddr);
    }
    pseudoClock_sem = 0;
}

HIDDEN void timerint(unsigned int IP){
    state_t *proc_state = (state_t *)BIOSDATAPAGE;
    if(IP == LOCALTIMERINT){
        updatePLT;
        currentProcess->p_s = *proc_state;
        currentProcess->p_time += TIMESLICE;
        insertProcQ(&readyQueue,currentProcess);
        dispatch();
    } else if (IP == TIMERINTERRUPT){
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


