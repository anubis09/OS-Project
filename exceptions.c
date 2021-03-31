#include "initialize.h"
#include "../include/include.h"






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
        process = outChild(process);
        if(process->p_semAdd != NULL ){
            process->p_semAdd++;
        }
        processCount--;
    }
}





/*SYS8*/
/*return the value of p_supportStruct from the current process*/
HIDDEN support_t* get_support_data(){
    return currentProcess->p_supportStruct;
}


