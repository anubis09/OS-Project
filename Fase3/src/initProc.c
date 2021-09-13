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

mutualExclusion_Semaphore *getSupDevSem(int devType, int devNum, int isReceiver)
{
    return &supportDeviceSemaphores[(devType + isReceiver) * 8 + (devNum - 1)];
}

HIDDEN void initProc(int asid)
{
    memaddr ramTop;
    RAMTOP(ramTop);
    memaddr exceptStack = ramTop - (2 * asid * PAGESIZE);

    state_t pstate;
    pstate.pc_epc = pstate.reg_t9 = UPROCSTARTADDR;
    pstate.reg_sp = USERSTACKTOP;
    pstate.status |= TEBITON | IMON | IEPON | USERPON;
    pstate.entry_hi = asid << ASIDSHIFT;
    /*init sup struct*/
    supStructVector[asid - 1].sup_asid = asid;
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr)pageFaultHandler;
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr)exceptStack;
    supStructVector[asid - 1].sup_exceptContext[PGFAULTEXCEPT].c_status = IMON | IEPON | TEBITON;
    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr)generalExceptionHandler;
    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr)(exceptStack + PAGESIZE);
    supStructVector[asid - 1].sup_exceptContext[GENERALEXCEPT].c_status = IMON | IEPON | TEBITON;

    /*bisogna inizializzare anche la tabella delle pagine!!*/
    /*fare la sys1*/
}

void istantiatorProcess()
{
    initSuppSem();
    initSwapStructs();
    /*mettere qualcosa!*/
}
