#ifndef ASL_H
#define ASL_H

#include "pandos_const.h"
#include "pandos_types.h"

/*semaphore descriptor type*/
typedef struct semd_t {
    struct semd_t *s_next; /*next element on the ASL*/
    int  *s_semAdd; /*pointer to the semaphore*/
    pcb_PTR  s_procQ; /*tail pointer to a process queue */
} semd_t;

void initASL();

/*inizializza il semaforo passato come primo parametro
con i valori passati come semAdd e next */
HIDDEN void initSemaphore(semd_t *,int *,semd_t *);

/*cerca nella ASL per un descrittore del semaforo, se lo trova lo ritorna
senno ritorna il semaforo nella ASL con descrittore di valore inferiore
più vicino a quello dato in input*/
HIDDEN semd_t *searchSem(int *);

/*prende un semaforo da lista libera 
lo inizializza con semAdd e lo restituisce
se lista libera è vuota restituisce NULL*/
HIDDEN semd_t *allocSem(int *semAdd);

/*prende il semaforo con valore semAdd da ASL e lo 
restituisce alla lista libera.
per ora nonn ci sono controlli che non sia un dummy node*/
HIDDEN void freeSem(int *);
int insertBlocked(int *, pcb_PTR );
pcb_PTR removeBlocked(int *);
pcb_PTR outBlocked(pcb_PTR );
pcb_PTR headBlocked(int *);

#endif