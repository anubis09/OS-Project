#include "interrupts.h"

void interruptHandler(){
    populateDevRegister(drA);
    unsigned int causeCode = 0xFF00 & getCAUSE();

    while (causeCode != 0x0000){
        if(causeCode == LOCALTIMERINT || causeCode == TIMERINTERRUPT) timerint(causeCode);
        else externalDeviceint(causeCode);
    }
}

void timerint(unsigned int IP){
    if(IP == LOCALTIMERINT){
        setTIMER(DEFAULTTIMER);
        state_t *proc_state = (state_t *)BIOSDATAPAGE;
        currentProcess->p_s = *proc_state;
        insertProcQ(&readyQueue,currentProcess);
        dispatch();
    } else if (IP == TIMERINTERRUPT){
        state_t *proc_state = (state_t *)BIOSDATAPAGE;
        SYSCALL (WAITCLOCK, 0, 0, 0);
        LDIT(time);
        VClock(proc_state);
    }
}

void VClock(state_t *proc_state){ 
    int *semaddr = &pseudoClock_sem;
    pcb_PTR *fp;
    fp = removeBlocked(semaddr);
    int block = FALSE;
    
    while(fp!=NULL){
        fp->p_semAdd=NULL;
        insertProcQ(&readyQueue, fp);
    }
    pseudoClock_sem = NULL;

    LDST(BIOSDATAPAGE);
    retControl(proc_state,block); 
    
}

void externalDeviceint(unsigned int IP){
    unsigned int idbmA;
    int ioStatus, *semaddr = NULL;
    
    if(IP == DISKINTERRUPT){
        idbmA = (int* )0x10000040;
        *semaddr = &disk_sem[checkdevNo(idbmA)];

        acknowledge(DISKINT,idbmA);
        ioStatus = SYSCALL (WAITIO,DISKINT,checkdevNo(idbmA), FALSE);
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
    }
    else if(IP == FLASHINTERRUPT){
        idbmA = (int* )0x10000040 + 0x04;
        *semaddr = &flash_sem[checkdevNo(idbmA)];

        acknowledge(FLASHINT,idbmA); 
        ioStatus = SYSCALL (WAITIO,FLASHINT,checkdevNo(idbmA), FALSE);
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
    }
    else if(IP == NETWORKINTERRUPT){
        idbmA = (int* )0x10000040 + 0x08;
        *semaddr = &network_sem[checkdevNo(idbmA)];

        acknowledge(NETWINT,idbmA);
        ioStatus = SYSCALL (WAITIO,NETWINT,checkdevNo(idbmA), FALSE);
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
    }
    else if(IP == PRINTINTERRUPT){
        idbmA = (int* )0x10000040 + 0x0C;
        *semaddr = &printer_sem[checkdevNo(idbmA)];

        acknowledge(PRNTINT,idbmA); 
        ioStatus = SYSCALL (WAITIO,PRNTINT,checkdevNo(idbmA), FALSE);
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
    }
    else if(IP == TERMINTERRUPT){
        idbmA = (int* )0x10000040 + 0x10;
        acknowledge(TERMINT,idbmA);

        if(choiceterm(idbmA)){
            *semaddr = &transmitter_sem[checkdevNo(idbmA)];
            ioStatus = SYSCALL (WAITIO,TERMINT,checkdevNo(idbmA), FALSE);
        } 
        else {
            *semaddr = &receiver_sem[checkdevNo(idbmA)];
            ioStatus = SYSCALL (WAITIO,TERMINT,checkdevNo(idbmA), TRUE);
        }

        SYSCALL (VERHOGEN, *semaddr, 0, 0);
    }
}

int checkdevNo(unsigned int check){
    if((DEV0ON & check) == 0)
        return 0;
    else if((DEV1ON & check) == 1)
        return 1;
    else if((DEV2ON & check) == 2)
        return 2;
    else if((DEV3ON & check) == 3)
        return 3;
    else if((DEV4ON & check) == 4)
        return 4;
    else if((DEV5ON & check) == 5)
        return 5;
    else if((DEV6ON & check) == 6)
        return 6;   
    else if((DEV7ON & check) == 7)
        return 7;
}

void acknowledge(int interrupt, int idbmA){
    unsigned int statusdev, *ack = NULL;
    if(interrupt == TERMINT){
        if(choiceterm(idbmA)) ack = (int*) drA.devreg[interrupt-3][checkdevNo(idbmA)].term.transm_command;
        else ack = (int*) drA.devreg[interrupt-3][checkdevNo(idbmA)].term.recv_command;
    } 
    else ack = (int*) drA.devreg[interrupt-3][checkdevNo(idbmA)].dtp.command; 
    
    *ack = ACK;
}

HIDDEN int choiceterm(int idbmA){
    if(drA.devreg[TERMINT-3][checkdevNo(idbmA)].term.transm_status == 5) return TRUE;
    else if(drA.devreg[TERMINT-3][checkdevNo(idbmA)].term.recv_status == 5) return FALSE;
}