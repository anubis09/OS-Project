#include "pcb.h"

HIDDEN pcb_PTR pcbFree_h; /*pcbFree list head pointer*/

void initPcbs(){ 
    static pcb_t pcbFree_table[MAXPROC];
    /*from pcbFree_table we create a NULL-terminated single linearly linked list*/
    for (int i = 0; i < MAXPROC-1; i++){
        pcbFree_table[i].p_next = &pcbFree_table[i+1];
    }
    pcbFree_table[MAXPROC-1].p_next = NULL;
    pcbFree_h = &pcbFree_table[0];
}

void freePcb(pcb_PTR p){
    /*head insert into pcbFree list*/
    p->p_next = pcbFree_h;
    pcbFree_h = p;
}

pcb_PTR allocPcb(){
    pcb_PTR tmp = pcbFree_h;
    if(tmp != NULL) {
        /*the head of pcbFree list is removed, then initialized every fields to NULL or 0*/
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
        tmp->p_time = 0;
        tmp->semAdd = NULL;
    }
    return tmp;
}

/*Process Queue Maintenance*/

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
    if(emptyProcQ(*tp)){ 
        /*we create queue with only 1 pcb (p)*/
        p->p_next = p;
        p->p_prev = p;
        *tp = p;
    }
    else{ 
        /*tail inserting*/
        p->p_next = *tp;
        p->p_prev = (*tp)->p_prev;
        /*updating old tail pointer p_prev to point to p*/
        (*tp)->p_prev = p;
        /*updating head pointer p_next to point to the new tail pointer p*/       
        p->p_prev->p_next = p;
        *tp = p;
    }
}

/*remove an element from the queue pointed to by tp.
if element = *tp, we have to change *tp */
HIDDEN void removeEl(pcb_PTR *tp, pcb_PTR element){
    int equal = (element == *tp);
    if(equal && element == element->p_next){
        /*we are removing the only element in the queue*/
        *tp = NULL;
    }
    else{
        element->p_prev->p_next = element->p_next;
        element->p_next->p_prev = element->p_prev;
        if(equal){
            /*we need to update tail pointer because we removed the old one*/
            *tp = (*tp)->p_next;
        }
    }
}


pcb_PTR removeProcQ(pcb_PTR *tp){
    pcb_PTR head = NULL;
    if(!emptyProcQ(*tp)){
        head = (*tp)->p_prev;
        removeEl(tp,head);
    }
    return head;
}


pcb_PTR outProcQ(pcb_PTR *tp,pcb_PTR p){
    if(!emptyProcQ(*tp)){
        pcb_PTR tmp = *tp;
        do
        {
            if(tmp == p){
                /*now we know that p is inside the queue, and we can safely remove it*/
                removeEl(tp,p);
                return p;
            }
            tmp = tmp->p_next;
        } while (tmp != *tp);
        
    }
    return NULL; /*if queue is empty or p not in queue*/
}

pcb_PTR headProcQ(pcb_PTR tp){
    if(!emptyProcQ(tp)){
       return tp->p_prev;
    }
    else{
        return NULL;
    }
}

/*Process Tree Maintenance*/

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
        if(emptyChild(prnt)){
            prnt->p_child = p;
        }
        else{ 
            /*head inserting p in sibling lists */
            prnt->p_child->p_prev_sib = p;
            p->p_next_sib = prnt->p_child;
            prnt->p_child = p;
        }
    }
    /*this way we have sibling list as a NULL-terminated double, linearly linked list*/
}


pcb_PTR removeChild(pcb_PTR p){
    pcb_PTR child = NULL;
    if((p != NULL)&&(p->p_child != NULL)){
        /*save pcb to return*/
        child = p->p_child;
       /*removing pointers to parent and siblings*/
        child->p_prnt = NULL;
        p->p_child = child->p_next_sib;
        /*if child had a sibling, we have to update his prev pointer
        and child next sib pointer*/
        if(p->p_child != NULL){ 
            p->p_child->p_prev_sib = NULL;
            child->p_next_sib = NULL;
        }
    }
    return child;
}


pcb_PTR outChild(pcb_PTR p){
    if((p != NULL)&&(p->p_prnt != NULL)){
        if(p->p_prev_sib == NULL){ /*first son*/
            removeChild(p->p_prnt);
        }
        else if(p->p_next_sib == NULL){ /*last son in siblings list*/
            p->p_prnt = NULL;
            p->p_prev_sib->p_next_sib = NULL;
        }
        else{/*in the middle*/
            p->p_prnt = NULL;
            p->p_prev_sib->p_next_sib = p->p_next_sib;
            p->p_next_sib->p_prev_sib = p->p_prev_sib;
        }
        return p;
    }
    return NULL;/*p has no father or is NULL*/
}