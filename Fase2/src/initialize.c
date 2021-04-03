#include "initialize.h"
#include "p2test.c"
extern void test();

/*this function initializes all global variables to zero.*/
HIDDEN void initVar(){
    processCount = 0;
    softBlockCount = 0;
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;
    for(int i = 0; i < SEMAPHORENUM; i++){
        device_Semaphore[i] = 0;
    }
}

/*this function initialize a passupvector_t pointer*/
HIDDEN void iniPassUPvector(passupvector_t *puvec){
    puvec->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    puvec->tlb_refill_stackPtr = KERNELSTACK;
    puvec->exception_handler = (memaddr) exceptionshandler; 
    puvec->exception_stackPtr = KERNELSTACK;
}

int main(){
    /*populate pass up vector*/
    iniPassUPvector((passupvector_t *)PASSUPVECTOR);
    /*initialize variables*/
    initPcbs();
    initASL();
    initVar();
    /*loading 100millisec in the Interval Timer*/
    LDIT(PSECOND);
    /*creating the first process, adam*/
    pcb_PTR adam = allocPcb();
    adam->p_s.status = IEPON | IMON | TEBITON;
    RAMTOP(adam->p_s.reg_sp);
    adam->p_s.pc_epc = (memaddr) test;
    adam->p_s.reg_t9 = (memaddr) test;
    adam->p_prnt = NULL;
    adam->p_child = NULL;
    adam->p_next_sib = NULL;
    adam->p_prev_sib = NULL;
    adam->p_semAdd = NULL;
    adam->p_time = 0;
    adam->p_supportStruct = NULL;
    insertProcQ(&readyQueue,adam);
    processCount++;
    /*scheduler call*/
    dispatch();
    return 0;
}