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
    Terminate (SYS9)
    The SYS9 service is essentially a user-mode “wrapper” 
    for the kernel-mode restricted SYS2 service. 
*/
HIDDEN void terminate(int asid)
{
    swapCleanUp(asid);
    SYSCALL(VERHOGEN, (int)&masterSemaphore, 0, 0);
    SYSCALL(TERMPROCESS, 0, 0, 0);
}

/*
    GetTOD (SYS10)
    Returns the number of microseconds passed since the system was last booted/reset
*/
HIDDEN void getTod(state_t *procState)
{
    int tod;
    STCK(tod);
    procState->reg_v0 = tod;
}

/*
    WriteToPrinter (SYS11)
    Transmits a string of characters to the printer device associated with the U-proc.
    Returns the number of characters transmitted, if the write was successful, 
    otherwise returns the negative of the device's status value.
*/
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
        /*It is an error to write to a printer device from an address outside of 
        the request-ing U-proc’s logical address space, request a SYS11 with a 
        length less than 0, ora length greater than 128*/
        terminate(asid);
    }
}

/*
    WriteToTerminal (SYS12)
    Transmits a string of characters to the terminal device associated with the U-proc.
    Returns the number of characters transmitted, if the write was successful, 
    otherwise returns the negative of the device's status value.
*/
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
        /*It is an error to write to a terminal device from an address outside of 
        the request-ing U-proc’s logical address space, request a SYS11 with a 
        length less than 0, ora length greater than 128*/
        terminate(asid);
    }
}

/*
    ReadFromTerminal (SYS13)
    Receives a string of characters from the terminal device associated with the U-proc.
    Returns the number of characters received, if the write was successful, 
    otherwise returns the negative of the device's status value.
*/
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
        /*Attempting to read from a terminal device to an address outside
         of the request-ing U-proc’s logical address space is an error*/
        terminate(asid);
    }
}

void programTrap(int asid)
{
    terminate(asid);
}

/*
    supportSyscallDispatcher determines which type of user syscall was request,
    and passes control to the right handler.
*/
HIDDEN void supportSyscallDispatcher(support_t *sup_struct)
{
    state_t *procState = &sup_struct->sup_exceptState[GENERALEXCEPT];
    int asid = sup_struct->sup_asid;
    int sysType = procState->reg_a0;
    switch (sysType)
    {
    case TERMINATE:
        terminate(asid);
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
        programTrap(asid);
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
        programTrap(sup_struct->sup_asid);
        break;
    }
}