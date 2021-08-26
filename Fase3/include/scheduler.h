#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../include/include.h"

/*Macro for setting PLT to time slice*/
#define updatePLT setTIMER(TIMESLICE *(*((cpu_t *)TIMESCALEADDR)))

/*
    Preemptive round-robin scheduling algorithm with a time slice value of 5 milliseconds.
    If there are processes in the ready queue, dispatch the “next” process in the Ready Queue.
    If not:
    1) If the Process Count is zero invoke the HALT BIOS service. Consider this a job well done!
    2) If the Process Count>0 and the Soft-block Count>0 enter a Wait State.
    3) Deadlock is defined as when the Process Count>0 and the Soft-block Count is zero. invoke the PANIC BIOS service!
*/
void scheduler();

#endif