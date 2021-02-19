#ifndef PCB_H
#define PCB_H

#include "pandos_const.h"
#include "pandos_types.h"

/*Allocation and Deallocation of pcbs*/
void initPcbs();
void freePcb(pcb_PTR);
pcb_PTR allocPcb();

/*Process Queue Maintenance*/
pcb_PTR mkEmptyProcQ();
int emptyProcQ(pcb_PTR);
void insertProcQ(pcb_PTR *, pcb_PTR);
pcb_PTR removeProcQ(pcb_PTR *);
pcb_PTR outProcQ(pcb_PTR *, pcb_PTR);
pcb_PTR headProcQ(pcb_PTR );

/*Process Tree Maintenance*/
int emptyChild(pcb_PTR);
void insertChild(pcb_PTR, pcb_PTR);
pcb_PTR removeChild(pcb_PTR);
pcb_PTR outChild(pcb_PTR);

#endif