#ifndef ASL_H
#define ASL_H

#include "pandos_const.h"
#include "pandos_types.h"

/*semaphore descriptor type*/
typedef struct semd_t {
    struct semd_t *s_next; /*next element on the ASL*/
    int  *s_semAdd; /*pointer to the semaphore*/
    pcb_PTR  s_procQ; /*tail pointer to a process queue */
} semd_t;

void initASL();
int insertBlocked(int *, pcb_PTR );
pcb_PTR removeBlocked(int *);
pcb_PTR outBlocked(pcb_PTR );
pcb_PTR headBlocked(int *);

#endif