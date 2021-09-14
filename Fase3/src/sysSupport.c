#include "../include/sysSupport.h"
#define TERMSTATMASK 0xFF
#define TERMCHARRECVMASK 0xFF00

/*al posto di devNo bisogna mettere asid - 1 e al posto di intLineNo TERMINT per terminale*/
HIDDEN devregtr *getDeviceRegAddr(int intLineNo, int devNo)
{
    return (devregtr *)(DEVREGBASE + (intLineNo - STARTINTLINEDEVICE) * 0x80 + devNo * 0x10);
}

HIDDEN int isKuseg(unsigned int addr)
{
    if (addr >= KUSEG)
        return TRUE;
    else
        return FALSE;
}

HIDDEN void terminate()
{
    SYSCALL(VERHOGEN, (int)&masterSemaphore, 0, 0);
    SYSCALL(TERMPROCESS, 0, 0, 0);
}

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
        mutualExclusion_Semaphore *printerSem = getSupDevSem(PRINTER, asid, FALSE);
        printerSem->asidProcInside = asid;
        SYSCALL(PASSEREN, (int)&(printerSem->semVal), 0, 0);
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
        printerSem->asidProcInside = -1;
        SYSCALL(VERHOGEN, (int)&(printerSem->semVal), 0, 0);
    }
    else
    {
        terminate(asid);
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

        mutualExclusion_Semaphore *transmitterSem = getSupDevSem(TERMINAL, asid, FALSE);
        transmitterSem->asidProcInside = asid;
        SYSCALL(PASSEREN, (int)&(transmitterSem->semVal), 0, 0);

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
        transmitterSem->asidProcInside = -1;
        SYSCALL(VERHOGEN, (int)&(transmitterSem->semVal), 0, 0);
    }
    else
    {
        terminate(asid);
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
        mutualExclusion_Semaphore *receiverSem = getSupDevSem(TERMINAL, asid, TRUE);
        SYSCALL(PASSEREN, (int)&(receiverSem->semVal), 0, 0);
        receiverSem->asidProcInside = asid;
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
                if (c == '\n')
                {
                    isNotOver = FALSE;
                    procState->reg_v0 = nChar;
                }
                else
                {
                    *string = c;
                    string++;
                    nChar++;
                }
            }
        }
        receiverSem->asidProcInside = -1;
        SYSCALL(VERHOGEN, (int)&(receiverSem->semVal), 0, 0);
    }
    else
    {
        terminate();
    }
}

void programTrap(int asid)
{
    freeME(asid);
    terminate();
}

void supportSyscallDispatcher(support_t *sup_struct)
{
    state_t *procState = &sup_struct->sup_exceptState[GENERALEXCEPT];
    int asid = sup_struct->sup_asid;
    int sysType = procState->reg_a0;
    switch (sysType)
    {
    case TERMINATE:
        terminate();
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
    support_t *sup_struct = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    unsigned int cause = (sup_struct->sup_exceptState[GENERALEXCEPT].cause & GETEXECCODE) >> CAUSESHIFT;
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