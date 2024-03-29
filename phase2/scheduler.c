#include "../h/scheduler.h"
#define PROCEEDPATH 1
#define HALTPATH 2
#define WAITPATH 3
#define PANICPATH 4

/*
    Checks if the scheduler should dispatch a new process(PROCEEDPATH), 
    halt execution(HALTPATH), wait(WAITPATH), or panic(PANICPATH)
*/
HIDDEN int check()
{
    if (!emptyProcQ(readyQueue))
    {
        /*Ready queue isn't empty*/
        return PROCEEDPATH;
    }
    else
    {
        if (processCount == 0)
        {
            /*All processes are terminated*/
            return HALTPATH;
        }
        else if (processCount > 0 && softBlockCount > 0)
        {
            /*Some process are blocked on an I/O or timer request, so we wait for them*/
            return WAITPATH;
        }
        else
        {
            /*DEADLOCK processCount > 0 && softBlockCount == 0*/
            return PANICPATH;
        }
    }
}

/*
    For a wait operation, we have to ensure to have PC0 status with interrupt bits on, and PLT bit off
*/
HIDDEN void prepWait()
{
    unsigned int status = getSTATUS();
    /*Switch on interrupt bits*/
    status |= IECON | IMON;
    /*Turn off TE bit*/
    disable_timer(&status);
    /*Updating CP0 status*/
    setSTATUS(status);
}

void scheduler()
{
    int path = check();
    switch (path)
    {
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
        updatePLT;
        /*Scheduler is always called when the processor is in kernel mode.*/
        LDST(&(currentProcess->p_s));
        break;
    }
}
