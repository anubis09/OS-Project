#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../include/include.h"
#include "initialize.h"

/*
    Signed integer, indicates the time at which a process starts.
*/
signed int timeStart;

/*
    Preemptive round-robin scheduling algorithm with a time slice value of 5 milliseconds.
    If there are processes in the ready queue, dispatch the“next” process in the Ready Queue.
    If not:
    1) If the Process Count is zero invoke the HALT BIOS service. Consider this a job well done!
    2) If the Process Count>0 and the Soft-block Count>0 enter aWait State.
    3) Deadlock is defined as when the Process Count>0 and the Soft-block Count is zero. invoke the PANIC BIOS service!
*/
void dispatch();



#endif