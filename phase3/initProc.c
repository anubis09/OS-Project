#include "../h/initProc.h"

/*
    Support level structures usefull for process initialization.
*/
HIDDEN support_t supStructVector[UPROCMAX];

/*
    Initialize all the support device semaphores to 1.
*/
HIDDEN void initSuppSem()
{
    for (int i = 0; i < SUPDEVSEMNUM; i++)
    {
        supportDeviceSemaphores[i] = 1;
    }
}

int *getSupDevSem(int devType, int devNum, int isReceiver)
{
    return &supportDeviceSemaphores[(devType + isReceiver) * DEVPERINT + (devNum - 1)];
}

/*
    This function initializes one process.
    It takes as input the unique ASID of the new process.
*/
HIDDEN void initProc(int asid)
{
    /*this is useful for the direct allocation of the two stack spaces per U-proc,
    one for the Support Level’sTLB exception handler, and one for the Support Level’s general exceptionhandler*/
    memaddr ramTop;
    RAMTOP(ramTop);
    memaddr exceptStack = ramTop - (asid * PAGESIZE * 2);

    /*setting up the processor state for the new process*/
    state_t pstate;
    pstate.pc_epc = pstate.reg_t9 = UPROCSTARTADDR;
    pstate.reg_sp = USERSTACKTOP;
    pstate.status = TEBITON | IMON | IEPON | USERPON;
    pstate.entry_hi = asid << ASIDSHIFT;

    /*setting up sup struct for the new process*/
    supStructVector[asid - 1].sup_asid = asid;

    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr)pageFaultHandler;
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr)(exceptStack + PAGESIZE);
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_status = IMON | IEPON | TEBITON;

    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr)generalExceptionHandler;
    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr)exceptStack;
    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_status = IMON | IEPON | TEBITON;

    /*reading the U-procs header informations in the first block outside the swap pool*/

    memaddr info = swapStart + POOLSIZE * PAGESIZE;
    flashOperation(asid, 0, info, FLASHREAD);
    memaddr *textStart = (memaddr *)(info + 0x0008);
    memaddr *textSize = (memaddr *)(info + 0x0014);

    /*setting up the page table entry, all pages must be dirty, private and invalid*/
    for (int i = 0; i < MAXPAGES - 1; i++)
    {
        unsigned int vpn = 0x80000000 + (i * PAGESIZE);
        supStructVector[asid - 1].sup_privatePgTbl[i].pte_entryHI = vpn + (asid << ASIDSHIFT);
        if (vpn >= *textStart && vpn <= *textStart + *textSize)
        {
            /*settig all the U-proc's .text pages ad read only */
            supStructVector[asid - 1].sup_privatePgTbl[i].pte_entryLO = 0;
        }
        else
        {
            supStructVector[asid - 1].sup_privatePgTbl[i].pte_entryLO = DIRTYON;
        }
    }
    supStructVector[asid - 1].sup_privatePgTbl[MAXPAGES - 1].pte_entryHI = 0xBFFFF000 + (asid << ASIDSHIFT);
    supStructVector[asid - 1].sup_privatePgTbl[MAXPAGES - 1].pte_entryLO = DIRTYON;

    /*creating the new process with SYS1*/
    int status = SYSCALL(CREATEPROCESS, (int)&pstate, (int)&(supStructVector[asid - 1]), 0);
    if (status != OK)
    {
        /*we can't create the new process, so we kill the current process.*/
        SYSCALL(TERMPROCESS, 0, 0, 0);
    }
}

void istantiatorProcess()
{
    /*initiliazing support level data structures and semaphores.*/
    initSuppSem();
    initSwapStructs();
    masterSemaphore = 0;

    /*creating all the procs, every one with a unique ASID*/
    for (int asid = 1; asid <= UPROCMAX; asid++)
    {
        initProc(asid);
    }
    for (int i = 1; i <= UPROCMAX; i++)
    {
        SYSCALL(PASSEREN, (int)&masterSemaphore, 0, 0);
    }

    /*all processes are terminated, so we can go in system halted*/
    SYSCALL(TERMPROCESS, 0, 0, 0);
}