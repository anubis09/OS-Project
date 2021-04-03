#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../include/include.h"
#include "initialize.h"
#include "deviceregister.h"
#include "exceptions.h"

#define NETWORKINTERRUPT 0x00002000
void interruptHandler();

#endif