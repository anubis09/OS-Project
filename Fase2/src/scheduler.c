#include "scheduler.h"
#define time_perClock (5000 * (*((cpu_t *) TIMESCALEADDR)))
#define PROCEEDPATH 1
#define HALTPATH 2
#define WAITPATH 3
#define PANICPATH 4

HIDDEN int check(){
    if(!emptyProcQ(readyQueue)){
        /*ready queue isn't empty*/
        return PROCEEDPATH;
    }
    else{
        if(processCount == 0){
            return HALTPATH;
        }
        else if(processCount > 0 && softBlockCount > 0){
            return WAITPATH;
        }
        else{
            /*DEADLOCK processCount > 0 && soft>BlockCount == 0*/
            return PANICPATH;
        }
    }
}

void dispatch(){
    int path = check();
    unsigned int status = getSTATUS();
    switch (path){
    case HALTPATH:
        HALT();
        break;
    case WAITPATH:
        /*we have to ensure to have interrupt bits on, and PLT bit off*/
        /*switch on interrupt bits*/
        set_interruptsOn(&status);
        /*turn off TE bit*/
        disable_timer(&status);
        /*reset the status*/
        setSTATUS(status);
        WAIT();
        break;
    case PANICPATH:
        PANIC();
        break;
    default:
        currentProcess = removeProcQ(&readyQueue);
        setTIMER(time_perClock);
        set_kernelMode(status);
        setSTATUS(status);/*forse non serve*/
        LDST(&(currentProcess->p_s));
        break;
    }
}

