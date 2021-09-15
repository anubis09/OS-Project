#include "../include/vmSupport.h"

#define SWAPSTART 0x20020000

HIDDEN swap_t swapPool_table[POOLSIZE];
HIDDEN int swptSemaphore;

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
        swapPool_table[i].sw_pageNo = 0;
        swapPool_table[i].sw_pte = NULL;
    }
    swptSemaphore = 1;
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
    non so se il phisical address debba essere completo oppure no, ma io penso di si.
*/
HIDDEN void flashOperation(int asid, unsigned int page, memaddr physicalAddr, int command)
{
    /*cambiare come prendiamo il semaforo, ma quello lo famo successivamente*/
    int *flashSem = getSupDevSem(FLASH, asid, FALSE);
    SYSCALL(PASSEREN, (int)&flashSem, 0, 0);
    /*individuo il corretto flash device.*/
    memaddr *devRegAddr = (memaddr *)(DEVREGBASE + (0x80 + (asid - 1) * 0x10));
    *(devRegAddr + 2) = physicalAddr;
    atomicON();
    *(devRegAddr + 1) = (page << 8) | command;
    int statusCode = SYSCALL(IOWAIT, FLASHINT, asid - 1, FALSE);
    atomicOFF();
    SYSCALL(VERHOGEN, (int)&flashSem, 0, 0);
    if (statusCode != READY)
    {
        programTrap(&swptSemaphore);
    }
}

void pageFaultHandler()
{
    support_t *sup_struct = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    int asid = sup_struct->sup_asid;
    unsigned int cause = (sup_struct->sup_exceptState[PGFAULTEXCEPT].cause & GETEXECCODE) >> CAUSESHIFT;
    if (cause == MODIFICATIONEXCEPTION)
    {
        programTrap(NULL); //giuto con NULL
    }
    else
    {
        SYSCALL(PASSEREN, (int)&swptSemaphore, 0, 0);
        unsigned int entry_hi = sup_struct->sup_exceptState[PGFAULTEXCEPT].entry_hi;
        unsigned int page = ((entry_hi & GETPAGENO) >> VPNSHIFT) & 0xFF;
        if (page > 30)
            page = 31;
        /*visto che page non è in memoria prendiamo un frame a caso dalla mem fisica*/
        int offSet = getPointer();
        swap_t *entry = &(swapPool_table[offSet]);
        /*we find the PFN*/
        unsigned int lo = SWAPSTART + offSet * PAGESIZE;
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
            flashOperation(asid, page, lo, FLASHWRITE);
        }
        /*9 read the content of page p from CurrP backing store into the frame i.*/
        flashOperation(asid, page, lo, FLASHREAD);
        entry->sw_asid = asid;
        entry->sw_pageNo = entry_hi & GETPAGENO;
        entry->sw_pte = &sup_struct->sup_privatePgTbl[page];
        /*setting valid, global and dirty bits on.*/
        lo |= VALIDON | DIRTYON;
        atomicON();
        sup_struct->sup_privatePgTbl[page].pte_entryLO = lo;
        /*UPDATING TLB*/
        TLBCLR();
        atomicOFF();
        SYSCALL(VERHOGEN, (int)&swptSemaphore, 0, 0);
        LDST(&(sup_struct->sup_exceptState[PGFAULTEXCEPT]));
    }
}