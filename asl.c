#include "asl.h"
#include "pcb.h"
#define MAXMEM 0xFFFFFFFF
#define MINMEM 0X00000000

HIDDEN semd_t *semd_h; 
HIDDEN semd_t *semdFree_h;

/*PRE IDENTIFICARE UN SEMAFORO SI USA L'INDIRIZZO DI MEMORIA E NON IL VALORE.
UTILIZZO DI INDIRIZZO DI MEMORIA PER TENERLA ORDINATA
PROBLEMA È NELLA SEARCH, PERCHÈ NON HO UN MAXINT SUFFICIENTEMENTE GRANDE.*/

/*inizializza il semaforo passato come primo parametro
con i valori passati come semAdd e next */
static void initSemaphore(semd_t *semaphore, int *semAdd, semd_t *next){
    semaphore->s_semAdd = semAdd;
    semaphore->s_next = next;
    semaphore->s_procQ = mkEmptyProcQ();
}

/*cerca nella ASL per un descrittore del semaforo, se lo trova lo ritorna
senno ritorna il semaforo nella ASL con descrittore di valore inferiore
più vicino a quello dato in input*/
static semd_t *searchSem(int *semAdd){
    semd_t *tmp = semd_h;
    semd_t *tmp_prev = tmp;
    while((tmp->s_semAdd <= semAdd)){ /*appena ne trovo uno < mi fermo */
        tmp_prev = tmp;
        tmp = tmp->s_next;
    }
    /*tmp_prev è il semaforo con valore = a semAdd oppure minore,
    ma il minore più vicino, visto che la ASL è in ordine crescente*/
    return tmp_prev;
}

/*prende un semaforo da lista libera 
lo inizializza con semAdd e lo restituisce
se lista libera è vuota restituisce NULL*/
static semd_t *allocSem(int *semAdd){
    if(semdFree_h == NULL){
        return NULL;
    }
    else{
        semd_t *semaphore = semdFree_h;
        /*ora aggiorno lista vuota in modo che perda elemento in testa, poi lo inizializzo.*/
        semdFree_h = semdFree_h->s_next; 
        initSemaphore(semaphore,semAdd,NULL);
        return semaphore;
    }
}

/*prende il semaforo con valore semAdd da ASL e lo 
restituisce alla lista libera.
per ora non ci sono controlli che non sia un dummy node*/
static void freeSem(int *semAdd){
    /*trovo il semaforo precedenti in ASL a quello con descrittore semAdd
    che essendo un valore intero basta diminuirlo di uno siamo a posto*/
    semd_t *prev_semaphore = searchSem(semAdd-1);
    semd_t *semaphore =prev_semaphore->s_next; 
    prev_semaphore->s_next = semaphore->s_next;
    /*ora basta inserire semaphore in testa alla lista libera.
    i valori di semaphore li cambio quando alloco da lista libera*/
    semaphore->s_next = semdFree_h;
    semdFree_h = semaphore;
}


void initASL(){
    static semd_t semdFree_table[MAXPROC+2]; /*per i dummy node*/
    for(int i = 1; i < MAXPROC; i++){
        semdFree_table[i].s_next = &semdFree_table[i+1];
    }
    semdFree_table[MAXPROC].s_next = NULL;
    /*in questo modo è una NULL-terminated single linearly linked list*/
    semdFree_h = &semdFree_table[1];
    /*ora che la lista dei semafori liberi è completa, inizializzo la ASL*/
    int *dummy1 =(int *)MINMEM, *dummy2 =(int *)MAXMEM;
    initSemaphore(&semdFree_table[0],dummy1,&semdFree_table[MAXPROC+1]);
    initSemaphore(&semdFree_table[MAXPROC+1],dummy2,NULL);
    /*ora la lista ASL ha i suoi due dummy node iniziali.*/
    semd_h = &semdFree_table[0];
}

int insertBlocked(int *semAdd, pcb_PTR p){
    int ret_val = FALSE;
    semd_t *semaphore = searchSem(semAdd);
    if(semaphore->s_semAdd == semAdd){
        p->semAdd = semAdd;
        insertProcQ(&(semaphore->s_procQ),p);
    }
    else{
        semd_t *new_semaphore = allocSem(semAdd);
        if(new_semaphore == NULL){
            ret_val = TRUE; /*lista libera è vuota*/
        }
        else{
            /*inserisco il nuovo semaforo al posto giusto nella ASL
            e il posto giusto è dopo semaphore, per come lo trovo*/
            new_semaphore->s_next = semaphore->s_next;
            semaphore->s_next = new_semaphore;
            p->semAdd = semAdd;
            insertProcQ(&(new_semaphore->s_procQ),p);
        }
    }
    return ret_val;
}

pcb_PTR removeBlocked(int *semAdd){
    semd_t *semaphore = searchSem(semAdd);
    /*se non ho semaforo con tale descrittore in ASL */
    if(semaphore->s_semAdd != semAdd){
        return NULL;
    }
    else{
        pcb_PTR head = removeProcQ(&(semaphore->s_procQ));
        /*se la lista diventa vuota, tolgo il semaforo dalla ASL*/
        if(emptyProcQ(semaphore->s_procQ)){
            freeSem(semAdd);
        }
        return head;
    }
}

pcb_PTR outBlocked(pcb_PTR p){
    semd_t *semaphore = searchSem(p->semAdd);
    return outProcQ(&(semaphore->s_procQ),p);
}

pcb_PTR headBlocked(int *semAdd){
    semd_t *semaphore = searchSem(semAdd);
    /*se non ho semaforo con tale descrittore in ASL */
     if(semaphore->s_semAdd != semAdd){
        return NULL;
    }
    else{
        /*ritorno la testa della coda, se è vuota ritorna NULL*/
        return headProcQ(semaphore->s_procQ);
    }
}

