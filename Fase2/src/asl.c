#include "../include/asl.h"
#include "../include/pcb.h"
#define MAXMEM 0xFFFFFFFF
#define MINMEM 0X00000000

/*semaphore descriptor type*/
typedef struct semd_t {
    struct semd_t *s_next; /*next element on the ASL*/
    int  *s_semAdd; /*pointer to the semaphore*/
    pcb_PTR  s_procQ; /*tail pointer to a process queue */
} semd_t;

HIDDEN semd_t *semd_h; /*ASL*/
HIDDEN semd_t *semdFree_h; /*semaphore free list*/

/*Inizialize semaphore */
HIDDEN void initSemaphore(semd_t *semaphore, int *semAdd, semd_t *next){
    semaphore->s_semAdd = semAdd;
    semaphore->s_next = next;
    semaphore->s_procQ = mkEmptyProcQ();
}

/*looking into ASL for semaphore address(semAdd), if found, returns semaphore
 otherwise returns the closest sempahore with a lower address*/
HIDDEN semd_t *searchSem(int *semAdd){
    semd_t *tmp = semd_h, *tmp_prev;
    /*using a do-while becuse the first node of semd_h is a dummy node with
    the lowest addressable memory address*/
    do
    {
        tmp_prev = tmp;
        tmp = tmp->s_next;
    } while (tmp->s_semAdd <= semAdd);
    /*tmp_prev it's the semaphore with address equal(or closest lower) to semAdd,
    considerting that ASL is sorted in ascending order*/
    return tmp_prev;
}

/*inizialize and return a semaphore from semdFree_h.
if semdFree_h is empty, returns NULL*/
HIDDEN semd_t *allocSem(int *semAdd){
    semd_t *semaphore = semdFree_h;
    if(semaphore != NULL){
        /*update semdFree_h so that it loses the first element*/
        semdFree_h = semdFree_h->s_next; 
        /* initializion of the new semaphore so that it loses the old information
        since they can be reused.*/
        initSemaphore(semaphore,semAdd,NULL);
    }
    return semaphore;
}

/*freeSem search into ASL the semaphore pointed to by semAdd and
returns it to the free list. We assume the semaphore to remove is inside ASL*/
HIDDEN void freeSem(int *semAdd){
    /*find the previous sempahore in ASl, that has to have a memory 
    address <= semAdd-1*/
    semd_t *prev_semaphore = searchSem(semAdd-1);
    semd_t *semaphore =prev_semaphore->s_next; 
    prev_semaphore->s_next = semaphore->s_next;
    /*now we head insert semaphore into the free list.*/
    semaphore->s_next = semdFree_h;
    semdFree_h = semaphore;
}


void initASL(){
    static semd_t semdFree_table[MAXPROC+2]; /*for dummy nodes*/
    /*from semdFree_table we create a NULL-terminated single linearly linked list
    leaving out the first and last element, that will be our dummy node for ASL*/
    for(int i = 1; i < MAXPROC; i++){
        semdFree_table[i].s_next = &semdFree_table[i+1];
    }
    semdFree_table[MAXPROC].s_next = NULL;
    semdFree_h = &semdFree_table[1];
    /*initialization of ASL*/
    initSemaphore(&semdFree_table[0],(int *)MINMEM,&semdFree_table[MAXPROC+1]);
    initSemaphore(&semdFree_table[MAXPROC+1],(int *)MAXMEM,NULL);
    semd_h = &semdFree_table[0];
}

int insertBlocked(int *semAdd, pcb_PTR p){
    int insertNotDone = FALSE;
    semd_t *semaphore = searchSem(semAdd);
    if(semaphore->s_semAdd == semAdd){ /*found the right semaphore*/
        p->p_semAdd = semAdd;
        insertProcQ(&(semaphore->s_procQ),p);
    }
    else{
        semd_t *new_semaphore = allocSem(semAdd);
        if(new_semaphore == NULL){
            insertNotDone = TRUE; /*free list is empty*/
        }
        else{
            /*inserting the nwe semaphore in the right place (so after
            semaphore, because of how is structured searchSem)*/
            new_semaphore->s_next = semaphore->s_next;
            semaphore->s_next = new_semaphore;
            p->p_semAdd = semAdd;
            insertProcQ(&(new_semaphore->s_procQ),p);
        }
    }
    return insertNotDone;
}

pcb_PTR removeBlocked(int *semAdd){
    semd_t *semaphore = searchSem(semAdd);
    if(semaphore->s_semAdd != semAdd){
        return NULL; /*semaphore not in ASL*/
    }
    else{
        pcb_PTR head = removeProcQ(&(semaphore->s_procQ));
        /*if procQ becomes empty, remove semaphore from ASL*/
        if(emptyProcQ(semaphore->s_procQ)){
            freeSem(semAdd);
        }
        return head;
    }
}

pcb_PTR outBlocked(pcb_PTR p){
    semd_t *semaphore = searchSem(p->p_semAdd);
    if(semaphore->s_semAdd != p->p_semAdd){
        return NULL; /*semaphore not in ASL*/
    }
    else{
        /*remove and returns p. If not inside process queue, returns null*/
        return outProcQ(&(semaphore->s_procQ),p);
    }
}

pcb_PTR headBlocked(int *semAdd){
    semd_t *semaphore = searchSem(semAdd);
     if(semaphore->s_semAdd != semAdd){
        return NULL; /*semaphore not in ASL*/
    }
    else{
        /*returns head of process queues. If empty returns null*/
        return headProcQ(semaphore->s_procQ);
    }
}