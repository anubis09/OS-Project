#include "interrupts.h"

void VClock(){ 
    int *semaddr = &pseudoClock_sem;
    pcb_PTR blocked_process = removeBlocked(semaddr);
    
    while(blocked_process!=NULL){
        blocked_process->p_semAdd=NULL;
        insertProcQ(&readyQueue, blocked_process);
        blocked_process = removeBlocked(semaddr);
    }
    pseudoClock_sem = 0;
}

int checkdevNo(unsigned int check){
    if((DEV0ON & check) == 0b00000001)
        return 0;
    else if((DEV1ON & check) == 0b00000010)
        return 1;
    else if((DEV2ON & check) == 0b00000100)
        return 2;
    else if((DEV3ON & check) == 0b00001000)
        return 3;
    else if((DEV4ON & check) == 0b00010000)
        return 4;
    else if((DEV5ON & check) == 0b00100000)
        return 5;
    else if((DEV6ON & check) == 0b01000000)
        return 6;   
    else /*if((DEV7ON & check) == 0b10000000)*/
        return 7;
}

HIDDEN int choiceterm(int idbmA){
    if(drA.devreg[TERMINT-3][checkdevNo(idbmA)].term.transm_status == 5) return TRUE;
    else /*if(drA.devreg[TERMINT-3][checkdevNo(*idbmA)].term.recv_status == 5)*/ return FALSE;
}

void acknowledge(int interrupt, int idbmA){
    unsigned int *ack = NULL;
    if(interrupt == TERMINT){
        if(choiceterm(idbmA)) ack = (unsigned int*) drA.devreg[interrupt-3][checkdevNo(idbmA)].term.transm_command;
        else ack = (unsigned int*) drA.devreg[interrupt-3][checkdevNo(idbmA)].term.recv_command;
    } 
    else ack = (unsigned int*) drA.devreg[interrupt-3][checkdevNo(idbmA)].dtp.command; 
    
    *ack = ACK;
}

void externalDeviceint(unsigned int IP){
    int *idbmA;
    int ioStatus, *semaddr = NULL;
    
    if((IP & DISKINTERRUPT) == DISKINTERRUPT){
        idbmA = (int *)0x10000040;
        semaddr = &disk_sem[checkdevNo(*idbmA)];
        ioStatus = drA.devreg[DISKINT-3][checkdevNo(*idbmA)].dtp.status;


        acknowledge(DISKINT,*idbmA); 
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
        
    }
    else if((IP & FLASHINTERRUPT) == FLASHINTERRUPT){
        idbmA = (int *)0x10000040 + 0x04;
        semaddr = &flash_sem[checkdevNo(*idbmA)];
        ioStatus = drA.devreg[FLASHINT-3][checkdevNo(*idbmA)].dtp.status;

        acknowledge(FLASHINT,*idbmA); 
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
        readyQueue->p_s.status = ioStatus;
    }
    else if((IP & NETWORKINTERRUPT) == NETWORKINTERRUPT){
        idbmA = (int *)0x10000040 + 0x08;
        semaddr = &network_sem[checkdevNo(*idbmA)];
        ioStatus = drA.devreg[FLASHINT-3][checkdevNo(*idbmA)].dtp.status;

        acknowledge(NETWINT,*idbmA);
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
        readyQueue->p_s.status = ioStatus;
    }
    else if((IP & PRINTINTERRUPT) == PRINTINTERRUPT){
        idbmA = (int *)0x10000040 + 0x0C;
        semaddr = &printer_sem[checkdevNo(*idbmA)];
        ioStatus = drA.devreg[PRNTINT-3][checkdevNo(*idbmA)].dtp.status;

        acknowledge(PRNTINT,*idbmA); 
        SYSCALL (VERHOGEN, *semaddr, 0, 0);
        readyQueue->p_s.status = ioStatus;
    }
    else if((IP & TERMINTERRUPT) == TERMINTERRUPT){
        idbmA = (int *)0x10000040 + 0x10;
        acknowledge(TERMINT,*idbmA);

        if(choiceterm(*idbmA)){
            semaddr = &transmitter_sem[checkdevNo(*idbmA)];
            ioStatus = drA.devreg[TERMINT-3][checkdevNo(*idbmA)].term.transm_status;
        } 
        else {
            semaddr = &receiver_sem[checkdevNo(*idbmA)];
            ioStatus = drA.devreg[TERMINT-3][checkdevNo(*idbmA)].term.recv_status;
        }

        SYSCALL (VERHOGEN, *semaddr, 0, 0);
        readyQueue->p_s.status = ioStatus;
    }
}

void timerint(unsigned int IP){
    if(IP == LOCALTIMERINT){
        updatePLT;
        state_t *proc_state = (state_t *)BIOSDATAPAGE;
        currentProcess->p_s = *proc_state;
        insertProcQ(&readyQueue,currentProcess);
        dispatch();
    } else if (IP == TIMERINTERRUPT){
        state_t *proc_state = (state_t *)BIOSDATAPAGE;
        LDIT(PSECOND);
        /*should unblock all pcbs on the semaphore.*/
        VClock();
        LDST(proc_state);
        /*DA GESTIRE IL CASO IN CUI IL PROCESSORE È IN WAIT()->COSA VIENE SALVATO IN BIOSDATAPAGE?*/
    }
}

void interruptHandler(){    
    populateDevRegister(drA);
    unsigned int causeCode = 0xFF00 & getCAUSE();

    if((causeCode & LOCALTIMERINT) == LOCALTIMERINT) {
        timerint(causeCode & LOCALTIMERINT);
    }
    else if((causeCode & TIMERINTERRUPT) == TIMERINTERRUPT) {
        timerint(causeCode & TIMERINTERRUPT);
    }
    else {
        externalDeviceint(causeCode);
    }
}
