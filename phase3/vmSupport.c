#include "../h/vmSupport.h"

HIDDEN swap_t swapPool_table[POOLSIZE];
HIDDEN int swptSemaphore;
memaddr swapStart;

void initSwapStructs()
{
    /*setting all frames as unoccupied*/
    for (int i = 0; i < POOLSIZE; i++)
    {
        swapPool_table[i].sw_asid = -1;
        swapPool_table[i].sw_pageNo = 0;
        swapPool_table[i].sw_pte = NULL;
    }
    swptSemaphore = 1;

    /*reading .core file to locate the start of the swap pool*/
    memaddr *textStart = (memaddr *)0x20001008;
    memaddr *textSize = (memaddr *)0x20001014;
    memaddr *dataSize = (memaddr *)0x20001024;
    swapStart = *textStart + *textSize + *dataSize;
}

void swapCleanUp(int asid)
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        if (swapPool_table[i].sw_asid == asid)
        {
            /*setting the frame as unoccupied.*/
            swapPool_table[i].sw_asid = -1;
        }
    }
}

/*
    Page replace algorithm.
    We have O(n) operations in exchange for few I/O (write) operations. 
*/
HIDDEN int getFrameIndex()
{
    static int pointer = 0;
    int found = FALSE;
    /*first checks for an unoccupied frame to use.*/
    for (int i = 0; i < POOLSIZE; i++)
    {
        if (swapPool_table[(pointer + i) % POOLSIZE].sw_asid == -1)
        {
            pointer = (pointer + i) % POOLSIZE;
            found = TRUE;
            break;
        }
    }
    if (!found)
    {
        /*all frames are occupied, so chooses the next on the array.*/
        pointer = (pointer + 1) % POOLSIZE;
    }
    return pointer;
}

void atomicON()
{
    unsigned int status = getSTATUS();
    status &= DISABLEINTS;
    setSTATUS(status);
}

void atomicOFF()
{
    unsigned int status = getSTATUS();
    status |= IECON;
    setSTATUS(status);
}

/*
    Search into the TLB if there is a entry with entry_hi == entry, if so
    that particular entry is rewritten.
    Entry is the entry_hi to look for, lo is the entry_lo to rewrite.
*/
HIDDEN void updateTLB(memaddr entry, memaddr lo)
{
    setENTRYHI(entry);
    TLBP();
    if ((getINDEX() & PRESENTFLAG) == 0)
    {
        setENTRYLO(lo);
        TLBWI();
    }
    /*not in the tlb, we don't have to update it.*/
}

/*
    This functions is for writing or reading from a flash device.
    Asid is the asid of the process for getting the right FLASH device.
    Page is the device block number.
    Pfn is the starting physical address of the 4k block.
    Command is for the proper operation to perform ( READ WRITE)
*/
HIDDEN void flashOperation(int asid, unsigned int page, memaddr pfn, int command)
{
    int *flashSem = getSupDevSem(FLASH, asid, FALSE);
    SYSCALL(PASSEREN, (int)&flashSem, 0, 0);
    /*finding the correct flash device*/
    memaddr *devRegAddr = GETDEVREGADDR(FLASHINT, asid - 1);
    /*writing the DATA0 register*/
    *(devRegAddr + 2) = pfn;
    atomicON();
    /*writing the command register */
    *(devRegAddr + 1) = (page << 8) | command;
    /*the I/O opertion has started, so we call a SYS5*/
    int statusCode = SYSCALL(IOWAIT, FLASHINT, asid - 1, FALSE);
    atomicOFF();
    SYSCALL(VERHOGEN, (int)&flashSem, 0, 0);
    if (statusCode != READY) /*error on the I/O operation*/
    {
        /*Firstly release the mutual exclusione on the swap semahore*/
        SYSCALL(VERHOGEN, (int)&swptSemaphore, 0, 0);
        /*Then we kill the process.*/
        programTrap(asid);
    }
}

void pageFaultHandler()
{
    support_t *sup_struct = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    int asid = sup_struct->sup_asid;
    unsigned int cause = (sup_struct->sup_exceptState[PGFAULTEXCEPT].cause & GETEXECCODE) >> CAUSESHIFT;
    if (cause == MODIFICATIONEXCEPTION)
    {
        /*modification exception are handled like program trap.*/
        programTrap(asid);
    }
    else
    {
        SYSCALL(PASSEREN, (int)&swptSemaphore, 0, 0);
        unsigned int entry_hi = sup_struct->sup_exceptState[PGFAULTEXCEPT].entry_hi;
        /*page is the device block number to read.*/
        unsigned int page = GETPAGE(entry_hi);
        if (page > 30)
            page = 31; /*that's the stack page index*/
        /*choose a frame*/
        int index = getFrameIndex();
        swap_t *entry = &(swapPool_table[index]);
        /*find the PFN*/
        unsigned int lo = swapStart + index * PAGESIZE;
        if (entry->sw_asid != -1)
        {
            /*This frame is occupied, we assume it's occupied by logical page k, 
            belonging to process x(ASID) and that is dirty.*/
            atomicON();
            /*make the frame invalid and update the tlb if necessary 
            (cache could be out of date)*/
            entry->sw_pte->pte_entryLO &= VALIDOFF;
            updateTLB(entry->sw_pte->pte_entryHI, entry->sw_pte->pte_entryLO);
            atomicOFF();
            /*update process x backing store*/
            flashOperation(entry->sw_asid, GETPAGE(entry->sw_pte->pte_entryHI), entry->sw_pte->pte_entryLO & GETPFN, FLASHWRITE);
        }
        /*read the content of page p from current proc backing store into the frame i.*/
        flashOperation(asid, page, lo, FLASHREAD);
        entry->sw_asid = asid;
        entry->sw_pageNo = entry_hi & GETPAGENO;
        entry->sw_pte = &sup_struct->sup_privatePgTbl[page];
        /*setting valid, and dirty bits on.*/
        lo |= VALIDON | DIRTYON;
        atomicON();
        sup_struct->sup_privatePgTbl[page].pte_entryLO = lo;
        /*UPDATING TLB*/
        updateTLB(entry_hi, lo);
        atomicOFF();
        SYSCALL(VERHOGEN, (int)&swptSemaphore, 0, 0);
        LDST(&(sup_struct->sup_exceptState[PGFAULTEXCEPT]));
    }
}