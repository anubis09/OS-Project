#include "../include/vmSupport.h"

#define SWAPSIZE UPROCMAX * 2
#define SWAPSTART 0x20020000

HIDDEN swap_t swapPoll_table[SWAPSIZE];
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
        swapPoll_table[i].sw_asid = -1;
        swapPoll_table[i].sw_pageNo = 0;
        swapPoll_table[i].sw_pte = NULL;
    }
}