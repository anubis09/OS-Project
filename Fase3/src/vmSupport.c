#include "../include/vmSupport.h"

#define SWAPSIZE UPROCMAX * 2
#define MODIFICATIONEXCEPTION 1
#define LOADEXCEPTION 2
#define STOREEXCEPTION 3
#define SWAPSTART 0x20020000
#define ALLON 0xFFFFFFFF

HIDDEN swap_t swapPool_table[SWAPSIZE];
HIDDEN int swptSemaphore = 1;

/*
Quando un proceso viene inizializzato, nella sua support struct deve mettere nel
contesto PC l'indirizzo delle funzioni per pagefault e generalexceptions.
*/

/*PAG 70 pops, COSA IMPORTANTE PER CAPIRE QUESTO CAPITOLO.*/

/*
ricordiamo che un processo verrà inizializzato con tutte le sue vpn come invalid.
invalid vuol dire che nessuna di queste sta in RAM.
quindi all'inizio i frame della swap pool devono essere tutti non assegnati.
*/
void initSwapStructs()
{
    for (int i = 0; i < SWAPSIZE; i++)
    {
        swapPool_table[i].sw_asid = -1;
        /*ESSENDO IL VPN, NON DEVO SETTARE DEI BIT STRANI.*/
        swapPool_table[i].sw_pageNo = 0;
        swapPool_table[i].sw_pte = NULL;
    }
}

HIDDEN int getPointer()
{
    static int pointer = -1;
    pointer = (pointer + 1) % SWAPSIZE;
    return pointer;
}

HIDDEN void atomicON()
{
    unsigned int status = getSTATUS();
    status &= (IECON ^ ALLON);
    setSTATUS(status);
}

HIDDEN void atomicOFF()
{
    unsigned int status = getSTATUS();
    status |= IECON;
    setSTATUS(status);
}

void pageFaultHandler()
{
    support_t *sup_struct = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    unsigned int cause = (sup_struct->sup_exceptState[PGFAULTEXCEPT].cause & GETEXECCODE) >> CAUSESHIFT;
    if (cause == MODIFICATIONEXCEPTION)
    {
        /*PROGRAM TRAP.*/
    }
    else
    {
        SYSCALL(PASSEREN, (int)&swptSemaphore, 0, 0);
        unsigned int entry_hi = sup_struct->sup_exceptState[PGFAULTEXCEPT].entry_hi;
        unsigned int p = (entry_hi & GETPAGENO); /*no VPN shift*/
        /*visto che p non è in memoria prendiamo un frame a caso dalla mem fisica*/
        int pointer = getPointer();
        swap_t *entry = &(swapPool_table[pointer]);
        if (entry->sw_asid != -1)
        {
            /*
                This frame is occupied, we assume it's occupied by logical page n k, 
                belonging to proc x(ASID) and that is dirty.
            */
            atomicON();
            entry->sw_pte->pte_entryLO &= (VALIDON ^ 0xFFFFFFFF);
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
        }
        /*9 read the content of page p from CurrP backing store into the frame i.*/
        entry->sw_asid = sup_struct->sup_asid;
        entry->sw_pageNo = p; /*non sono sicuro che sia giusto*/
        entry->sw_pte = sup_struct->sup_privatePgTbl;
        atomicON();
        /*we find the PFN*/
        unsigned int lo = SWAPSTART + pointer * PAGESIZE;
        /*setting valid, global and dirty bits on.*/
        lo |= VALIDON | GLOBALON | DIRTYON;
        /*we are now getting the index of the user page table array.*/
        unsigned int index = (p >> VPNSHIFT) & 0xFF;
        sup_struct->sup_privatePgTbl[index].pte_entryLO = lo;
        /*UPDATING TLB*/
        TLBCLR();
        atomicOFF();
        SYSCALL(VERHOGEN, (int)&swptSemaphore, 0, 0);
        LDST(&(sup_struct->sup_exceptState[PGFAULTEXCEPT]));
    }
}