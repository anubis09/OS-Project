#include "include.h"

void set_kernelMode(unsigned int *status){
    /*we just need to set to 0 the second bit of the status register
    so we need to use AND bitwiseso that we just change that bit.*/
    *status &= 0b111111111111111111111111111111101;
}

void set_userMode(unsigned int *status){
    /*we just need to set to 1 the second bit of the status register
    so we need to use OR bitwise, so that we just change that bit.*/
    *status |= 0b10;
}

void set_interruptsOn(unsigned int *status){
    /*we only need to set to 1 bits from 8 to 15 and bit 0,
    so we use OR bitwise, just to change them */
    *status |= IMON | IECON;
}

void disable_interrupts(unsigned int *status){
    /*we only need to set to 0 bits from 8 to 15 and bit 0,
    so we use AND bitwise, just to change them */
    *status &= 0b11111111111111110000000011111110;
}

void set_timerOn(unsigned int *status){
    /*we only need to set to 1 the 27th bit.*/
    *status |= TEBITON;
}

void disable_timer(unsigned int *status){
    /*we only need to set to 0 the 27th bit.*/
    *status &= 0b11110111111111111111111111111111;
}

int isKernelModeC(unsigned int status){
    unsigned int checkKernelMode = 0b00000000000000000000000000000010;
    checkKernelMode &= status;
    /*with AND bitwise, we will only check the second bit, because is the only bit in checkKernelModeC set to 1.
    if checkKernelMode == 0 then the second bit in status is 0, and the processor is in kernel mode.
    Otherwise it's 1 and the processor is in user mode.*/
   if (checkKernelMode == 0){
       return TRUE;
    }
   else{
       return FALSE;
    }
}

int isKernelModeP(unsigned int status){
    unsigned int checkKernelMode = 0b00000000000000000000000000001000;
     checkKernelMode &= status;
    /*with AND bitwise, we will only check the fourth bit, because is the only bit in checkKernelModeP set to 1.
    if checkKernelMode == 0 then the fourth bit in status is 0, and the processor is in kernel mode.
    Otherwise it's 1 and the processor is in user mode.*/
   if (checkKernelMode == 0){
       return TRUE;
    }
   else{
       return FALSE;
    }
}

void *memcpy(void *src, void *dest){
    int len = sizeof(src);
    char *d = dest;
    const char *s = src;
    while (len--)
      *d++ = *s++;
    return dest;
}

