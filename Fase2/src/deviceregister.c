#include "deviceregister.h"

HIDDEN void initDevreg(devreg_t devreg[DEVINTNUM][DEVPERINT]){
    for(int i=0; i< DEVINTNUM; i++){
        for(int j=0; j<DEVPERINT; j++){
            devreg[i][j].dtp.status = 0;
            devreg[i][j].dtp.status = 0;
            devreg[i][j].dtp.command = 0;
            devreg[i][j].dtp.command = 0;
            devreg[i][j].dtp.command = 0;
        
            devreg[i][j].term.recv_status = 0;
            devreg[i][j].term.recv_command = 0;
            devreg[i][j].term.transm_status = 0;
            devreg[i][j].term.transm_command = 0;
            
        }
    }
}
HIDDEN void initVec(unsigned int vec[], int length){
    for(int i=0; i<length;i++){
        vec[i] = 0;
    }
}
HIDDEN void initDrarea(devregarea_t drA){
    drA.rambase = 0;
    drA.ramsize = 0;
    drA.execbase = 0;
    drA.execsize = 0;
    drA.bootbase = 0;
    drA.bootsize = 0;
    drA.todhi = 0;
    drA.todlo = 0;
    drA.intervaltimer = 0;
    drA.timescale = 0;
    drA.tlb_floor_addr = 0;
    initVec(drA.inst_dev,5);
    initVec(drA.interrupt_dev,5);
    initDevreg(drA.devreg);
}
unsigned int pAdd(unsigned int a){
    unsigned int* add;
    add = (unsigned int* ) a;
    return *add;
}

void populateDevRegister(){
    unsigned int devAddrBase;
    initDrarea(drA);
    
    drA.rambase = pAdd(RAMBASEADDR);
    drA.ramsize = pAdd(RAMBASESIZE);
    drA.execbase = pAdd(0x10000008);
    drA.execsize = pAdd(0x1000000C);
    drA.bootbase = pAdd(0x10000010);
    drA.bootsize = pAdd(0x10000014);
    drA.todhi = pAdd(0x10000018);
    drA.todlo = pAdd(0x1000001C);
    drA.intervaltimer = pAdd(INTERVALTMR);
    drA.timescale = pAdd(TIMESCALEADDR);
    drA.tlb_floor_addr = pAdd(0x10000028);
    drA.inst_dev[5] = pAdd(0x1000002C);
    drA.interrupt_dev[5] = pAdd(0x10000040);

    for(int i=0; i< DEVINTNUM; i++){
        for(int j=0; j<DEVPERINT; j++){
            devAddrBase = 0x10000054 + ((i - 3) * 0x80)+ (j * 0x10);
            if(i < DEVINTNUM -1){
                drA.devreg[i][j].dtp.status = pAdd(devAddrBase + 0x0);
                drA.devreg[i][j].dtp.command = pAdd(devAddrBase + 0x4);
                drA.devreg[i][j].dtp.command = pAdd(devAddrBase + 0x8);
                drA.devreg[i][j].dtp.command = pAdd(devAddrBase + 0xc);
            }  else {
                drA.devreg[i][j].term.recv_status = pAdd(devAddrBase + 0x0);
                drA.devreg[i][j].term.recv_command = pAdd(devAddrBase + 0x4);
                drA.devreg[i][j].term.transm_status = pAdd(devAddrBase + 0x8);
                drA.devreg[i][j].term.transm_command = pAdd(devAddrBase + 0xc);
            }
        }
    }
}
