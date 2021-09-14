#include "../include/initProc.h"

HIDDEN support_t supStructVector[UPROCMAX];

HIDDEN void initSuppSem()
{
    for (int i = 0; i < SUPDEVSEMNUM; i++)
    {
        supportDeviceSemaphores[i].semVal = 1;
        supportDeviceSemaphores[i].asidProcInside = -1;
    }
}

/*devNUm = asid - 1*/
mutualExclusion_Semaphore *getSupDevSem(int devType, int devNum, int isReceiver)
{
    return &supportDeviceSemaphores[(devType + isReceiver) * DEVPERINT + (devNum)];
}

HIDDEN void initProc(int asid)
{
    memaddr ramTop;
    RAMTOP(ramTop);
    memaddr exceptStack = ramTop - (asid * PAGESIZE * 2);

    state_t pstate;
    pstate.pc_epc = pstate.reg_t9 = UPROCSTARTADDR;
    pstate.reg_sp = USERSTACKTOP;
    pstate.status = TEBITON | IMON | IEPON | USERPON;
    pstate.entry_hi = asid << ASIDSHIFT;
    /*init sup struct*/
    supStructVector[asid - 1].sup_asid = asid;
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr)pageFaultHandler;
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr)(exceptStack + PAGESIZE);
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_status = IMON | IEPON | TEBITON;

    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr)generalExceptionHandler;
    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr)exceptStack;
    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_status = IMON | IEPON | TEBITON;

    /*bisogna inizializzare anche la tabella delle pagine!!*/
    for (int i = 0; i < MAXPAGES - 1; i++)
    {
        supStructVector[asid - 1].sup_privatePgTbl[i].pte_entryHI = 0x80000000 + (i * PAGESIZE) + (asid << ASIDSHIFT);
        supStructVector[asid - 1].sup_privatePgTbl[i].pte_entryLO = DIRTYON | GLOBALON;
    }
    supStructVector[asid - 1].sup_privatePgTbl[MAXPAGES - 1].pte_entryHI = 0xBFFFF000 + (asid << ASIDSHIFT);
    supStructVector[asid - 1].sup_privatePgTbl[MAXPAGES - 1].pte_entryLO = DIRTYON | GLOBALON;
    /*fare la sys1*/
    int status = SYSCALL(CREATEPROCESS, (int)&pstate, (int)&(supStructVector[asid - 1]), 0);
    if (status != OK)
    {
        SYSCALL(TERMPROCESS, 0, 0, 0);
    }
}

void istantiatorProcess()
{
    initSuppSem();
    initSwapStructs();
    masterSemaphore = 0;

    for (int asid = 1; asid <= UPROCMAX; asid++)
    {
        initProc(asid);
    }
    for (int i = 1; i <= UPROCMAX; i++)
    {
        SYSCALL(PASSEREN, (int)&masterSemaphore, 0, 0);
    }
    SYSCALL(TERMPROCESS, 0, 0, 0);
}
