
#include "../h/initialize.h"

/*This function initializes all global variables to zero.*/
HIDDEN void initVar()
{
    processCount = 0;
    softBlockCount = 0;
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;
    for (int i = 0; i < SEMAPHORENUM; i++)
    {
        device_Semaphore[i] = 0;
    }
}

/*This function initialize a passupvector_t pointer*/
HIDDEN void initPassUPvector(passupvector_t *puvec)
{
    puvec->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    puvec->tlb_refill_stackPtr = KERNELSTACK;
    puvec->exception_handler = (memaddr)exceptionsHandler;
    puvec->exception_stackPtr = KERNELSTACK;
}

void main()
{
    /*Populate pass up vector*/
    initPassUPvector((passupvector_t *)PASSUPVECTOR);
    /*Initialize variables*/
    initPcbs();
    initASL();
    initVar();
    /*Loading 100millisec in the Interval Timer*/
    LDIT(PSECOND);
    /*Creating the first process, adam*/
    pcb_PTR adam = allocPcb();
    /*Interrupts  enabled,  the  processorLocal Timer enabled,  kernel-mode on*/
    adam->p_s.status = IEPON | IMON | TEBITON;
    RAMTOP(adam->p_s.reg_sp);
    adam->p_s.pc_epc = (memaddr)istantiatorProcess;
    adam->p_s.reg_t9 = (memaddr)istantiatorProcess;
    adam->p_prnt = NULL;
    adam->p_child = NULL;
    adam->p_next_sib = NULL;
    adam->p_prev_sib = NULL;
    adam->p_semAdd = NULL;
    adam->p_time = 0;
    adam->p_supportStruct = NULL;
    insertProcQ(&readyQueue, adam);
    processCount++;
    scheduler();
}