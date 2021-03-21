#include "../include/asl.h"
#include "../include/pcb.h"
#include "p2test.c"
#define time 100000
extern void test();

HIDDEN int processCount;
HIDDEN int softBlockCount;
HIDDEN pcb_PTR readyQueue;
HIDDEN pcb_PTR currentProcess;

HIDDEN int pseudoClock_sem;
HIDDEN int disk_sem[8];
HIDDEN int flash_sem[8];
HIDDEN int network_sem[8];
HIDDEN int printer_sem[8];
/*terminal devices semaphore*/
HIDDEN int transmitter_sem[8];
HIDDEN int receiver_sem[8];

/*Pass Up Vector*/
typedef struct passupvector { 
	unsigned int tlb_refill_handler;
	unsigned int tlb_refill_stackPtr;
	unsigned int exception_handler;
	unsigned int exception_stackPtr; 
} passupvector_t;


HIDDEN void initSem(int sem[],int length){
    for(int i=0; i<length;i++){
        sem[i] = 0;
    }
}

HIDDEN void initVar(){
    processCount = 0;
    softBlockCount = 0;
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;
    pseudoClock_sem = 0;
    initSem(disk_sem,8);
    initSem(flash_sem,8);
    initSem(network_sem,8);
    initSem(printer_sem,8);
    initSem(transmitter_sem,8);
    initSem(receiver_sem,8);
}

HIDDEN void iniPassUPvector(passupvector_t *puvec){
    puvec->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    puvec->tlb_refill_stackPtr = KERNELSTACK;
    /*puvec->exception_handler = (memaddr) fooBar;*/
    puvec->exception_stackPtr = KERNELSTACK;
}

int main(){
    /*populate pass up vector*/
      
    /*initialize variables*/
    initPcbs();
    initASL();
    initVar();
    /*loading 100millisec in the Interval Timer*/
    LDIT(time);
    /*creating the first process, adam*/
    pcb_PTR adam = allocPcb();
    insertProcQ(&readyQueue,adam);
    processCount++;
    adam->p_s.status = 0b00011000000000001111111100000000 ;
    unsigned int ram;
    RAMTOP(ram);
    adam->p_s.gpr[26] = ram; 
    adam->p_s.pc_epc = (memaddr) test;
    adam->p_s.gpr[24] = (memaddr) test;
    adam->p_child = NULL;
    adam->p_next_sib = NULL;
    adam->p_prev_sib = NULL;
    adam->p_prnt = NULL;
    adam->p_semAdd = NULL;
    adam->p_time = 0;
    adam->p_supportStruct = NULL;
    /*scheduler call*/
}
