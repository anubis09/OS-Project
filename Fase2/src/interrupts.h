#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#define NETWORKINTERRUPT 0x00002000
#define DEFAULTTIMER 5000

#include "../include/include.h"
#include "initialize.c"
#include "deviceregister.h"
#include "exceptions.h"

void interruptHandler();

#endif