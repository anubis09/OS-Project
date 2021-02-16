#include "pcb.h"

HIDDEN pcb_PTR pcbFree_h; 

void initPcbs(){ 
    static pcb_t pcbFree_table[MAXPROC];
    //ora inizializzo i puntatori a next di ogni pcb, in modo da creare una linked list
    for (int i = 0; i < MAXPROC-1; i++){
        pcbFree_table[i].p_next = &pcbFree_table[i+1];
    }
    pcbFree_table[MAXPROC-1].p_next = NULL;
    pcbFree_h = &pcbFree_table[0];
}

void freePcb(pcb_PTR p){
    //semplice inserimento in testa.
    p->p_next = pcbFree_h;
    pcbFree_h = p;
}

pcb_PTR allocPcb(){
    pcb_PTR tmp = pcbFree_h;
    if(tmp != NULL) {
        //il primo nodo viene eliminato dalla lista e poi inizializzato con NULL o 0
        pcbFree_h = pcbFree_h->p_next;
        tmp->p_next = NULL;
        tmp->p_prev = NULL;
        tmp->p_prnt = NULL;
        tmp->p_child = NULL;
        tmp->p_next_sib = NULL;
        tmp->p_prev_sib = NULL;
        tmp->p_s.entry_hi = 0;
        tmp->p_s.cause = 0;
        tmp->p_s.status = 0;
        tmp->p_s.pc_epc = 0;
        for(int iterator = 0; iterator < STATE_GPR_LEN; iterator++){
            tmp->p_s.gpr[iterator] = 0;
        }
        tmp->p_s.hi = 0;
        tmp->p_s.lo = 0;
        tmp->semAdd = NULL;
    }
    return tmp;
}

//Process Queue Maintenance

pcb_PTR mkEmptyProcQ(){
    return NULL;
}

int emptyProcQ(pcb_PTR tp){
    if(tp == NULL){
        return TRUE;
    }
    else{
        return FALSE;
    }
}

void insertProcQ(pcb_PTR *tp, pcb_PTR p){
    if(emptyProcQ(*tp)){ //se tp vuoto creo la coda
        p->p_next = p;
        p->p_prev = p;
        *tp = p;
    }
    else{ //sennò aggiungo in coda
        p->p_next = *tp;
        p->p_prev = (*tp)->p_prev;
        //faccio in modo che il vecchio tail pointer punti con prev al nuovo tail pointer
        (*tp)->p_prev = p;
        //aggiorno puntatore next del primo elemento della coda al nuovo tail pointer
        p->p_prev->p_next = p;
        *tp = p;
    }
}

HIDDEN void removeEl(pcb_PTR element){
    //elemento precedente a quello da eliminare punta al nodo dopo element
    element->p_prev->p_next = element->p_next;
    //elemento successivo a quello da eliminare punta al nodo precedente element
    element->p_next->p_prev = element->p_prev;
}

pcb_PTR removeProcQ(pcb_PTR *tp){
    pcb_PTR head = NULL;
    if(!emptyProcQ(*tp)){
        head = (*tp)->p_prev;
        if(head == *tp){ //se la coda ha un solo elemento testa = tail pointer
            *tp = NULL;
        }
        else {
            removeEl(head);
        }
    }
    return head;
}

pcb_PTR outProcQ(pcb_PTR *tp,pcb_PTR p){
    if(!emptyProcQ(*tp)){
        pcb_PTR tmp = *tp;//serve per scorrere la coda
        do
        {
            if(tmp == p){
                if(tmp == *tp){
                    if(tmp->p_next == tmp){ //se coda ha un solo elemento
                        *tp = NULL;
                    }
                    else{//coda ha più di un elemento ma devo rimuovere il tail pointer
                        removeEl(tmp);
                        *tp = (*tp)->p_next;
                    }
                }
                else{ //rimuovo un elemento in mezzo alla coda
                    removeEl(tmp);
                }
                return p;
            }
            tmp = tmp->p_next;
        } while (tmp != *tp);
        
    }
    return NULL; //se tp è vuota oppure se p non è nella lista
}

pcb_PTR headProcQ(pcb_PTR tp){
    if(!emptyProcQ(tp)){
       return tp->p_prev;
    }
    else{
        return NULL;
    }
}

//Process Tree Maintenance

int emptyChild(pcb_PTR p){
    if(p->p_child == NULL){
         return TRUE;
    }
    else { 
        return FALSE;
    }
}


void insertChild(pcb_PTR prnt, pcb_PTR p){
    if((prnt != NULL)&&(p != NULL)){
        p->p_prnt = prnt;
        if(prnt->p_child == NULL){
            prnt->p_child = p;
        }
        else{ //se ha almeno un figlio bisogna scorrere la lista dei siblings
            pcb_PTR tmp = prnt->p_child;
            while(tmp->p_next_sib != NULL){
                tmp = tmp->p_next_sib;
            } 
            //inserisco p alla fine della lista dei siblings
            tmp->p_next_sib = p;
            p->p_prev_sib = tmp;
        }
    }
}


pcb_PTR removeChild(pcb_PTR p){
    pcb_PTR child = NULL;
    if((p != NULL)&&(p->p_child != NULL)){
        //salvo il nodo da ritornare
        child = p->p_child;
        //tolgo il puntatore parent dal child da rimuovere
        child->p_prnt = NULL;
        //cambio puntatore al primo figlio al parent, e lo metto al secondo
        p->p_child = child->p_next_sib;
        //dobbiamo aggiornare anche il puntatore a _prev_sib
        //ma solo se il nuovo child è diverso da NULL, sennò causa errore
        if(p->p_child != NULL){ 
            p->p_child->p_prev_sib = NULL;
        }
    }
    return child;
}


pcb_PTR outChild(pcb_PTR p){
    if((p != NULL)&&(p->p_prnt != NULL)){
        if(p->p_prev_sib == NULL){ //è il primo figlio
            removeChild(p->p_prnt);
        }
        else if(p->p_next_sib == NULL){ //se è ultimo nella lista dei siblings
            p->p_prnt = NULL;
            p->p_prev_sib->p_next_sib = NULL;
        }
        else{//se è in mezzo
            p->p_prnt = NULL;
            p->p_prev_sib->p_next_sib = p->p_next_sib;
            p->p_next_sib->p_prev_sib = p->p_prev_sib;
        }
        return p;
    }
    return NULL;//se p è NULL o se p non ha padre
}