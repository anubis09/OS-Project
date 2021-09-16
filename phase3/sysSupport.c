#include "../h/sysSupport.h"
#define TERMSTATMASK 0xFF
#define TERMCHARRECVMASK 0xFF00

/*
    Given the interrupt line number, and the device number, returns a pointer
    to the starting address location of the correct device register.
*/
HIDDEN devregtr *getDeviceRegAddr(int intLineNo, int devNo)
{
    return (devregtr *)(DEVREGBASE + (intLineNo - STARTINTLINEDEVICE) * 0x80 + devNo * 0x10);
}

/*
    Given a memory location, returns TRUE if is inside KUSEG, FALSE otherwise.
*/
HIDDEN int isKuseg(memaddr addr)
{
    if (addr >= KUSEG)
        return TRUE;
    else
        return FALSE;
}

/*
    The SYS9 service is essentially a user-mode “wrapper” 
    for the kernel-mode restricted SYS2 service. 
*/
HIDDEN void terminate(int *semaphore)
{
    if (semaphore != NULL)
    {
        /*release of mutual exclusion on support level semaphores.*/
        SYSCALL(VERHOGEN, (int)semaphore, 0, 0);
    }
    SYSCALL(VERHOGEN, (int)&masterSemaphore, 0, 0);
    SYSCALL(TERMPROCESS, 0, 0, 0);
}

/*
    Returns the number of microseconds passed since the system was last booted/reset
*/
HIDDEN void getTod(state_t *procState)
{
    int tod;
    STCK(tod);
    procState->reg_v0 = tod;
}

HIDDEN void writeToPrinter(state_t *procState, int asid)
{
    char *text = (char *)procState->reg_a1;
    int len = (int)procState->reg_a2;
    if (isKuseg((memaddr)text) == TRUE && len >= 0 && len <= MAXSTRLENG)
    {
        devregtr *printerReg = getDeviceRegAddr(PRNTINT, asid - 1);
        int status, nchar = 0;
        int *printerSem = getSupDevSem(PRINTER, asid, FALSE);
        SYSCALL(PASSEREN, (int)&printerSem, 0, 0);
        while (*text != EOS)
        {
            *(printerReg + 2) = (unsigned int)*text;
            atomicON();
            *(printerReg + 1) = PRINTCHAR;
            status = SYSCALL(IOWAIT, PRNTINT, asid - 1, FALSE);
            atomicOFF();

            if ((status & 0xFF) != READY)
            {
                procState->reg_v0 = -1 * (status & 0xFF);
                break;
            }
            text++;
            nchar++;
        }
        if (status == 1)
        {
            procState->reg_v0 = nchar;
        }
        SYSCALL(VERHOGEN, (int)&printerSem, 0, 0);
    }
    else
    {
        terminate(NULL);
    }
}

HIDDEN void writeToTerminal(state_t *procState, int asid)
{
    char *text = (char *)procState->reg_a1;
    int len = (int)procState->reg_a2;

    if (isKuseg((memaddr)text) == TRUE && len >= 0 && len <= MAXSTRLENG)
    {
        devregtr *termReg = getDeviceRegAddr(TERMINT, asid - 1);
        int status, nChar = 0;

        int *transmitterSem = getSupDevSem(TERMINAL, asid, FALSE);
        SYSCALL(PASSEREN, (int)&transmitterSem, 0, 0);

        while (*text != EOS)
        {

            atomicON();
            *(termReg + 3) = ((unsigned int)*text << BYTELENGTH) | TRANSMITCHAR;
            status = SYSCALL(IOWAIT, TERMINT, asid - 1, FALSE);
            atomicOFF();

            if ((status & TERMSTATMASK) != OKCHARRECV)
            {
                procState->reg_v0 = -1 * (status & TERMSTATMASK);
                break;
            }

            nChar++;
            text++;
        }

        if ((status & TERMSTATMASK) != OKCHARRECV)
        {
            procState->reg_v0 = nChar;
        }
        SYSCALL(VERHOGEN, (int)&transmitterSem, 0, 0);
    }
    else
    {
        terminate(NULL);
    }
}

HIDDEN void readFromTerminal(state_t *procState, int asid)
{
    char *string = (char *)procState->reg_a1;
    if (isKuseg((memaddr)string) == TRUE)
    {
        devregtr *base = getDeviceRegAddr(TERMINT, asid - 1);
        unsigned int status;
        int nChar = 0, isNotOver = TRUE;
        int *receiverSem = getSupDevSem(TERMINAL, asid, TRUE);
        SYSCALL(PASSEREN, (int)&receiverSem, 0, 0);
        while (isNotOver)
        {
            atomicON();
            *(base + 1) = RECEIVECHAR;
            status = SYSCALL(IOWAIT, TERMINT, asid - 1, TRUE);
            atomicOFF();
            if ((status & TERMSTATMASK) != OKCHARRECV)
            {
                procState->reg_v0 = -1 * (status & TERMSTATMASK);
                isNotOver = FALSE;
            }
            else
            {
                unsigned int c = (status & TERMCHARRECVMASK) >> BYTELENGTH;
                *string = c;
                string++;
                nChar++;
                if (c == '\n')
                {
                    isNotOver = FALSE;
                    procState->reg_v0 = nChar;
                }
            }
        }
        SYSCALL(VERHOGEN, (int)&receiverSem, 0, 0);
    }
    else
    {
        terminate(NULL);
    }
}

void programTrap(int *sem)
{
    terminate(sem);
}

HIDDEN void supportSyscallDispatcher(support_t *sup_struct)
{
    state_t *procState = &sup_struct->sup_exceptState[GENERALEXCEPT];
    int asid = sup_struct->sup_asid;
    int sysType = procState->reg_a0;
    switch (sysType)
    {
    case TERMINATE:
        terminate(NULL);
        break;
    case GET_TOD:
        getTod(procState);
        break;
    case WRITEPRINTER:
        writeToPrinter(procState, asid);
        break;
    case WRITETERMINAL:
        writeToTerminal(procState, asid);
        break;
    case READTERMINAL:
        readFromTerminal(procState, asid);
        break;
    default:
        /*syscode > 13*/
        programTrap(NULL);
        break;
    }
    procState->pc_epc += WORDLEN;
    LDST(procState);
}

void generalExceptionHandler()
{
    /*gettin the cause of the exception*/
    support_t *sup_struct = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    unsigned int cause = (sup_struct->sup_exceptState[GENERALEXCEPT].cause & GETEXECCODE) >> CAUSESHIFT;
    /*passing control to the correct handler.*/
    switch (cause)
    {
    case SYSEXCEPTION:
        supportSyscallDispatcher(sup_struct);
        break;
    default:
        programTrap(NULL);
        break;
    }
}