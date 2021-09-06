#include "../include/vmSupport.h"

#define SWAPSIZE UPROCMAX * 2
#define MODIFICATIONEXCEPTION 1
#define LOADEXCEPTION 2
#define STOREEXCEPTION 3
#define SWAPSTART 0x20020000

HIDDEN swap_t swapPool_table[SWAPSIZE];
HIDDEN int swptSemaphore = 1;

/*
Quando un proceso viene inizializzato, nella sua support struct deve mettere nel
contesto PC l'indirizzo delle funzioni per pagefault e generalexceptions.
*/

/*PAG 70 pops, COSA IMPORTANTE PER CAPIRE QUESTO CAPITOLO.*/

/*
ricordiamo che un processo verrà inizializzato con tutte le sue vpn come invalid.
invalid vuoò dire che nessuna di queste sta in RAM.
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
        unsigned int entry_hi = sup_struct->sup_exceptState->entry_hi;
        unsigned int missing_p = (entry_hi & GETPAGENO) >> VPNSHIFT;
        /*adesso dobbiamo pickare un frame e andiamo a vedere come cavolo si fa nel pome*/;
        int pointer = getPointer();
        swap_t entry = swapPool_table[pointer];
        if (entry.sw_asid != -1)
        {
            /*
                This frame is occupied, we assume it's occupied by logical page n k, 
                belonging to proc x(ASID) and that is dirty.
            */
            entry.sw_pte->pte_entryLO &(VALIDON ^ 0xFFFFFFFF);
            /*
                we xor validon with 0xFFFFFFFF, which means we are changing all
                bits on to off, and viceversa. so we can now set the valid bit off
                with just a bitwise and -> since only the bits referring to the
                valid bits are 0, all the others are 1.
            */
            //UPDATE TLB ATOMICALLY
            //UPDATE BACKING STORE
        }
        SYSCALL(VERHOGEN, (int)&swptSemaphore, 0, 0);
        LDST(&(sup_struct->sup_exceptState[PGFAULTEXCEPT]));
    }
}