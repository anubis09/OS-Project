#include "initialize.h"
#include "../include/include.h"



/*SYS1*/
/*create a new process as a child of the caller*/
HIDDEN void create_process(state_t *proc_state){
    pcb_PTR newProcess = allocPcb(); 
    int ret_val;
    if(newProcess == NULL) ret_val = -1;
    else{
        state_t *statep = (state_t *)proc_state->reg_a1;
        support_t *supportp = (support_t*)proc_state->reg_a2;
        newProcess->p_s.status = statep->status;
        newProcess->p_supportStruct = supportp;
        newProcess->p_time = 0;
        newProcess->p_semAdd = NULL;
        insertProcQ(&readyQueue, newProcess);
        insertChild(currentProcess, newProcess);
        processCount++;
        ret_val = 0;
    }
    proc_state->reg_v0 = ret_val;
}


/*SYS2*/
/*terminate the current process along with its progeny*/
HIDDEN void terminate_process(pcb_PTR process){    
    
    if(process != NULL){

        if(process->p_child != NULL){
            terminate_process(process->p_child);            
        }

        if(process->p_next_sib != NULL){
            terminate_process(process->p_next_sib);
        }
        
        freePcb(process);
        process = outProcQ(&readyQueue ,process);
        dispatch();
    }
}


/*SYS5*/
HIDDEN int wait_for_io_device(int IOCOMMAND, int intlNo, int dnum, int termRead){
    
}


/*SYS8*/
/*return the value of p_supportStruct from the current process*/
HIDDEN support_t* get_support_data(){
    return currentProcess->p_supportStruct;
}


