#include "asl.h"
#include "pcb.h"
#define MAXINT 0xFFFFFFFF

HIDDEN semd_t *semd_h, *semdFree_h;

void initASL(){
    static semd_t semdFree_table[MAXPROC+2]; //per i dummy node
    for(int i = 1; i < MAXPROC; i++){
        semdFree_table[i].s_next = &semdFree_table[i+1];
    }
    semdFree_table[MAXPROC].s_next = NULL;
    //in questo modo è una NULL-terminated single linearly linked list
    semdFree_h = &semdFree_table[1];
    //ora che la lista dei semafori liberi è completa, inizializzo la ASL
    int dummy1 = 0, dummy2 = MAXINT;
    initSemaphore(&semdFree_table[0],&dummy1,&semdFree_table[MAXPROC+1]);
    initSemaphore(&semdFree_table[MAXPROC+1],&dummy2,NULL);
    //ora la lista ASL ha i suoi due dummy node iniziali.
    semd_h = &semdFree_table[0];
}

HIDDEN void initSemaphore(semd_t *semaphore, int *semAdd, semd_t *next){
    semaphore->s_semAdd = semAdd;
    semaphore->s_next = next;
    semaphore->s_procQ = mkEmptyProcQ();
}

HIDDEN semd_t *allocSem(int *semAdd){
    if(semdFree_h == NULL){
        return NULL;
    }
    else{
        semd_t *semaphore = semdFree_h;
        //ora aggiorno lista vuota in modo che perda elemento in testa, poi lo inizializzo.
        semdFree_h = semdFree_h->s_next; 
        initSemaphore(semaphore,semAdd,NULL);
        return semaphore;
    }
}

HIDDEN semd_t *searchSem(int *semAdd){
    semd_t *tmp = semd_h;
    semd_t *tmp_prev = tmp;
    while(*(tmp->s_semAdd) < *semAdd){
        tmp_prev = tmp;
        tmp = tmp->s_next;
    }
    //tmp_prev è il semaforo con valore = a semAdd oppure minore,
    //ma il minore più vicino, visto che la ASL è in ordine crescente
    return tmp_prev;
}


int insertBlocked(int *semAdd, pcb_PTR p){
    int ret_val = FALSE;
    semd_t *semaphore = searchSem(semAdd);
    if(semaphore->s_semAdd == semAdd){
        insertProcQ(&(semaphore->s_procQ),p);
    }
    else{
        semd_t *new_semaphore = allocSem(semAdd);
        if(new_semaphore == NULL){
            ret_val = TRUE; //lista libera è vuota
        }
        else{
            //inserisco il nuovo semaforo al posto giusto nella ASL
            //e il posto giusto è dopo semaphore, per come lo trovo
            new_semaphore->s_next = semaphore->s_next;
            semaphore->s_next = new_semaphore;
        }
    }
    return ret_val;
}

pcb_PTR removeBlocked(int *semAdd){

}

pcb_PTR outBlocked(pcb_PTR p){

}

pcb_PTR headBlocked(int *semAdd){

}
