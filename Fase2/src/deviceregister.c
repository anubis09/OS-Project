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

void populateDevRegister(){
    unsigned int devAddrBase;
    initDrarea(drA);

    drA.rambase = RAMBASEADDR;
    drA.ramsize = RAMBASESIZE;
    drA.execbase = *(int *)0x10000008;
    drA.execsize = *(int *) 0x1000000C;
    drA.bootbase = *(int *) 0x10000010;
    drA.bootsize = *(int *) 0x10000014;
    drA.todhi = *(int *) 0x10000018;
    drA.todlo = *(int *) 0x1000001C;
    drA.intervaltimer = *(int *)INTERVALTMR;
    drA.timescale = *(int *)TIMESCALEADDR;
    drA.tlb_floor_addr = *(int *) 0x10000028;
    drA.inst_dev[5] = *(int*) 0x1000002C;
    drA.interrupt_dev[5] = *(int *) 0x10000040;

    for(int i=0; i< DEVINTNUM; i++){
        for(int j=0; j<DEVPERINT; j++){
            devAddrBase = 0x10000054 + ((i - 3) * 0x80)+ (j * 0x10);
            if(i < DEVINTNUM -1){
                drA.devreg[i][j].dtp.status = *(int *) (devAddrBase + 0x0);
                drA.devreg[i][j].dtp.command = *(int *) (devAddrBase + 0x4);
                drA.devreg[i][j].dtp.command = *(int *) (devAddrBase + 0x8);
                drA.devreg[i][j].dtp.command = *(int *) (devAddrBase + 0xc);
            }  else {
                drA.devreg[i][j].term.recv_status = *(int *) (devAddrBase + 0x0);
                drA.devreg[i][j].term.recv_command = *(int *) (devAddrBase + 0x4);
                drA.devreg[i][j].term.transm_status = *(int *) (devAddrBase + 0x8);
                drA.devreg[i][j].term.transm_command = *(int *) (devAddrBase + 0xc);
            }
        }
    }
}