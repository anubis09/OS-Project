#include "scheduler.h"
#define timeSlice_perClock (5000 * (*((cpu_t *) TIMESCALEADDR)))
#define PROCEEDPATH 1
#define HALTPATH 2
#define WAITPATH 3
#define PANICPATH 4

/*
    checks if the scheduler should dispatch a new process(PROCEEDPATH), halt execution(HALTPATH),
    wait(WAITPATH), or panic(PANICPATH)
*/
HIDDEN int check(){
    if(!emptyProcQ(readyQueue)){
        /*ready queue isn't empty*/
        return PROCEEDPATH;
    }
    else{
        if(processCount == 0){
            /*all processes are terminated*/
            return HALTPATH;
        }
        else if(processCount > 0 && softBlockCount > 0){
            /*some process are blocked on a I/O operation, so we wait for them*/
            return WAITPATH;
        }
        else{
            /*DEADLOCK processCount > 0 && softBlockCount == 0*/
            return PANICPATH;
        }
    }
}

/*
    For a wait operation, we have to ensure to have PC0 status with interrupt bits on, and PLT bit off
*/
HIDDEN void prepWait(){
    unsigned int status = getSTATUS();
    /*switch on interrupt bits*/
    status = status | IECON | IMON;
    /*turn off TE bit*/
    disable_timer(&status);
    /*updating CP0 status*/
    setSTATUS(status);
}

void dispatch(){
    int path = check();
    switch (path){
    case HALTPATH:
        HALT();
        break;
    case WAITPATH:
        prepWait();
        WAIT();
        break;
    case PANICPATH:
        PANIC();
        break;
    default:
        currentProcess = removeProcQ(&readyQueue);
        setTIMER(timeSlice_perClock);
        /*saving the start time of the process.*/
        STCK(timeStart);
        /*dispatch is always called when the processor is in kernel mode.*/
        LDST(&(currentProcess->p_s));
        break;
    }
}

