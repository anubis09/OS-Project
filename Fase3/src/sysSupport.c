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
    SYSCALL(TERMPROCESS, 0, 0, 0);
}

HIDDEN void getTod(state_t *procState)
{
    unsigned int tod;
    STCK(tod);
    procState->reg_v0 = tod;
}

HIDDEN void writeToPrinter(state_t *procState, int asid)
{
    char *text = (char *)procState->reg_a1;
    int len = (int)procState->reg_a2;

    if (isKuseg((memaddr)text) == TRUE && len >= 1 && len <= 128)
    {
        devregtr *printerReg = getDeviceRegAddr(PRNTINT, asid - 1);
        int status, nchar = 0;

        //SYSCALL(PASSEREN, 0, 0, 0);
        while (*text != EOS)
        {
            atomicON();
            *(printerReg + 1) = RECEIVECHAR;
            status = SYSCALL(IOWAIT, PRNTINT, asid - 1, TRUE);
            atomicOFF();

            if (status != 1)
            {
                break;
            }
            text++;
            nchar++;
        }
        if (status == 1)
        {
            procState->reg_v0 = nchar;
        }

        //SYSCALL(VERHOGEN, 0, 0, 0);
    }
    else
    {
        terminate();
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
        SYSCALL(PASSEREN, 0, 0, 0); /*DA MODIFIICARE CON IL SEMAFORO APPROPRIATO*/
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
                char c = (char)(status & TERMCHARRECVMASK);
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
        SYSCALL(VERHOGEN, 0, 0, 0); /*anche qui serve il semaforo fra*/
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
        /* code */
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