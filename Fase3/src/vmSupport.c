#include "../include/vmSupport.h"

#define SWAPSTART 0x20020000
#define ALLBITSON 0xFFFFFFFF

HIDDEN swap_t swapPool_table[POOLSIZE];
HIDDEN mutualExclusion_Semaphore swptSemaphore;

HIDDEN void initSuppSem()
{
    swptSemaphore.semVal = 1;
    swptSemaphore.asidProcInside = -1;
    for (int i = 0; i < SUPDEVSEMNUM; i++)
    {
        supportDeviceSemaphore[i].semVal = 1;
        supportDeviceSemaphore[i].asidProcInside = -1;
    }
}

void freeME(int asid)
{
    if (swptSemaphore.asidProcInside == asid)
    {
        SYSCALL(VERHOGEN, (int)&swptSemaphore.semVal, 0, 0);
    }
    for (int i = 0; i < SUPDEVSEMNUM; i++)
    {
        if (supportDeviceSemaphore[i].asidProcInside == asid)
        {
            SYSCALL(VERHOGEN, (int)&supportDeviceSemaphore[i].semVal, 0, 0);
        }
    }
}

/*PAG 70 pops, COSA IMPORTANTE PER CAPIRE QUESTO CAPITOLO.*/

/*
ricordiamo che un processo verrà inizializzato con tutte le sue vpn come invalid.
invalid vuol dire che nessuna di queste sta in RAM.
quindi all'inizio i frame della swap pool devono essere tutti non assegnati.
*/
void initSwapStructs()
{
    for (int i = 0; i < POOLSIZE; i++)
    {
        swapPool_table[i].sw_asid = -1;
        /*ESSENDO IL VPN, NON DEVO SETTARE DEI BIT STRANI.*/
        swapPool_table[i].sw_pageNo = 0;
        swapPool_table[i].sw_pte = NULL;
    }
    initSuppSem(); /*PER ORA RIMANE QUI MA POI LO SPOSTIAMO*/
}

HIDDEN int getPointer()
{
    static int pointer = -1;
    pointer = (pointer + 1) % POOLSIZE;
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
    La read legge il blocco collocato in blocknumber e lo copia al physicalAddr
    La write legge il blocco collocato in physicalAddr e lo copia al blocknumber
*/
HIDDEN void flashOperation(int asid, unsigned int flashAddr, int physicalAddr, int command)
{
    SYSCALL(PASSEREN, (int)&(supportDeviceSemaphore[FLASH].semVal), 0, 0);
    supportDeviceSemaphore[FLASH].asidProcInside = asid;
    /*individuo il corretto flash device.*/
    unsigned int *devRegAddr = (unsigned int *)(DEVREGBASE + (0x80 + (asid - 1) * 0x10));
    *(devRegAddr + 2) = physicalAddr;
    atomicON();
    *(devRegAddr + 1) = flashAddr | command;
    int statusCode = SYSCALL(IOWAIT, FLASHINT, asid - 1, 0);
    atomicOFF();
    supportDeviceSemaphore[FLASH].asidProcInside = -1;
    SYSCALL(VERHOGEN, (int)&(supportDeviceSemaphore[FLASH].semVal), 0, 0);
    if (statusCode >= 2 || statusCode == 0)
    {
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
        programTrap(asid);
    }
    else
    {
        SYSCALL(PASSEREN, (int)&swptSemaphore, 0, 0);
        swptSemaphore.asidProcInside = asid;
        unsigned int entry_hi = sup_struct->sup_exceptState[PGFAULTEXCEPT].entry_hi;
        unsigned int p = (entry_hi & GETPAGENO);
        /*visto che p non è in memoria prendiamo un frame a caso dalla mem fisica*/
        int pointer = getPointer();
        swap_t *entry = &(swapPool_table[pointer]);
        /*we find the PFN*/
        unsigned int lo = SWAPSTART + pointer * PAGESIZE;
        if (entry->sw_asid != -1)
        {
            /*
                This frame is occupied, we assume it's occupied by logical page n k, 
                belonging to proc x(ASID) and that is dirty.
            */
            atomicON();
            entry->sw_pte->pte_entryLO &= (VALIDON ^ ALLBITSON);
            /*
                we xor VALIDON with 0xFFFFFFFF, which means we are changing all
                bits 1 to 0, and viceversa. so we can now set the valid bit off
                with just a bitwise and -> since only the bits referring to the
                valid bits are 0, all the others are 1.
            */
            TLBCLR();
            atomicOFF();
            /*UPDATE BACKING STORE 8.C gli vado a salvare quello che ci aveva messo
            dentro che poi lo sovrascrivo.*/
            flashOperation(asid, p, lo, FLASHWRITE);
        }
        /*9 read the content of page p from CurrP backing store into the frame i.*/
        flashOperation(asid, p, lo, FLASHREAD);
        entry->sw_asid = asid;
        entry->sw_pageNo = p;
        /*we are now getting the index of the user page table array.*/
        unsigned int index = (p >> VPNSHIFT) & 0xFF;
        entry->sw_pte = &sup_struct->sup_privatePgTbl[index];
        atomicON();
        /*setting valid, global and dirty bits on.*/
        lo |= VALIDON | GLOBALON | DIRTYON;
        sup_struct->sup_privatePgTbl[index].pte_entryLO = lo;
        /*UPDATING TLB*/
        TLBCLR();
        atomicOFF();
        swptSemaphore.asidProcInside = -1;
        SYSCALL(VERHOGEN, (int)&swptSemaphore, 0, 0);
        LDST(&(sup_struct->sup_exceptState[PGFAULTEXCEPT]));
    }
}