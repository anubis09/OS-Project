#include "../h/sysSupport.h"
#define TERMSTATMASK 0xFF
#define TERMCHARRECVMASK 0xFF00

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
    /*get syscall's parameters*/
    char *text = (char *)procState->reg_a1;
    int len = (int)procState->reg_a2;
    if (isKuseg((memaddr)text) == TRUE && len >= 0 && len <= MAXSTRLENG)
    {
        /*get the printer starting address and the printer semaphore*/
        devregtr *printerReg = GETDEVREGADDR(PRNTINT, asid - 1);
        int *printerSem = getSupDevSem(PRINTER, asid, FALSE);
        int status, nchar = 0;
        SYSCALL(PASSEREN, (int)&printerSem, 0, 0);
        /*until i have printed the whole string.*/
        while (*text != EOS)
        {
            /*put character into DATA0 register.*/
            *(printerReg + 2) = *text;
            atomicON();
            /*put the printchar command into the COMMAND register*/
            *(printerReg + 1) = PRINTCHAR;
            status = SYSCALL(IOWAIT, PRNTINT, asid - 1, FALSE);
            atomicOFF();

            if ((status & 0xFF) != READY)
            {
                /*error happened*/
                procState->reg_v0 = -1 * (status & 0xFF);
                break;
            }
            text++;
            nchar++;
        }
        if (status == 1)
        {
            /*error didn't happen*/
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
    /*get syscall's parameters*/
    char *text = (char *)procState->reg_a1;
    int len = (int)procState->reg_a2;

    if (isKuseg((memaddr)text) == TRUE && len >= 0 && len <= MAXSTRLENG)
    {
        /*get the terminal starting address and the terminal semaphore*/
        devregtr *termReg = GETDEVREGADDR(TERMINT, asid - 1);
        int *transmitterSem = getSupDevSem(TERMINAL, asid, FALSE);
        int status, nChar = 0;
        SYSCALL(PASSEREN, (int)&transmitterSem, 0, 0);
        /*until i have printed the whole string.*/
        while (*text != EOS)
        {
            atomicON();
            /*put character and command into the TRANSM_COMMAND register*/
            *(termReg + 3) = (*text << BYTELENGTH) | TRANSMITCHAR;
            status = SYSCALL(IOWAIT, TERMINT, asid - 1, FALSE);
            atomicOFF();

            if ((status & TERMSTATMASK) != OKCHARRECV)
            {
                /*error happened*/
                procState->reg_v0 = -1 * (status & TERMSTATMASK);
                break;
            }

            nChar++;
            text++;
        }

        if ((status & TERMSTATMASK) != OKCHARRECV)
        {
            /*error didn't happen*/
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
    /*get syscall's parameter */
    char *string = (char *)procState->reg_a1;
    if (isKuseg((memaddr)string) == TRUE)
    {
        /*get the terminal starting address and the terminal semaphore*/
        devregtr *base = GETDEVREGADDR(TERMINT, asid - 1);
        int *receiverSem = getSupDevSem(TERMINAL, asid, TRUE);
        unsigned int status;
        int nChar = 0, isNotOver = TRUE;
        SYSCALL(PASSEREN, (int)&receiverSem, 0, 0);
        /*until wereceive a newline character or an error status.*/
        while (isNotOver)
        {
            atomicON();
            /*put the receive command into the command receiver terminal register*/
            *(base + 1) = RECEIVECHAR;
            status = SYSCALL(IOWAIT, TERMINT, asid - 1, TRUE);
            atomicOFF();
            if ((status & TERMSTATMASK) != OKCHARRECV)
            {
                /*error happened*/
                procState->reg_v0 = -1 * (status & TERMSTATMASK);
                isNotOver = FALSE;
            }
            else
            {
                /*error didn't happen*/
                char c = (status & TERMCHARRECVMASK) >> BYTELENGTH;
                *string = c;
                string++;
                nChar++;
                if (c == '\n')
                {
                    /*we received the newline charcater, so the operation is over.*/
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