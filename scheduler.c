#include "../include/include.h"
#include <umps3/umps/libumps.h>
#include "initialize.h"
#define time_perClock (5000 * (*((cpu_t *) TIMESCALEADDR)))
#define PROCEEDPATH 1
#define HALTPATH 2
#define WAITPATH 3
#define PANICPATH 4
/*cose da controllare sono: 
1) capire come non ridare il controllo al main. devo probabilmente leggere tutto il funzinamento del kernel. 
*/

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
            return PANICPATH;
        }
    }
}

void dispatch(){
    int path = check();
    unsigned int currentStatus;
    switch (path){
    case HALTPATH:
        HALT();
        break;
    case WAITPATH:
        /*we have to ensure to have interrupt bits on, and PLT bit off*/
        currentStatus = getSTATUS();
        /*switch on interrupt bits*/
        currentStatus = currentStatus | 0b1111111100000001;
        /*turn off TE bit*/
        currentStatus = currentStatus & 0b0111111111111111111111111111;
        WAIT();
        break;
    case PANICPATH:
        PANIC();
        break;
    default:
        currentProcess = removeProcQ(&readyQueue);
        setTIMER(time_perClock);
        LDST(&(currentProcess->p_s));
        break;
    }

}