#include "asl.c"
#include "pcb.c"



int main() { 
    int i = 0;
    int sem[MAXPROC];
    int onesem;
    pcb_t	*procp[MAXPROC], *p, *qa, *q, *firstproc, *lastproc, *midproc;
    qa = NULL;
    initPcbs();
    for (i = 0; i < MAXPROC; i++) {
		if ((procp[i] = allocPcb()) == NULL)
			printf("allocPcb: unexpected NULL   ");
	}
	if (allocPcb() != NULL) {
		printf("allocPcb: allocated more than MAXPROC entries   ");
	}
	printf("allocPcb ok   \n");


	for (i = 10; i < MAXPROC; i++)
		freePcb(procp[i]);
	printf("freed 10 entries   \n");
    if (!emptyProcQ(qa)) printf("emptyProcQ: unexpected FALSE   ");
	printf("Inserting...   \n");
	for (i = 0; i < 10; i++) {
		if ((q = allocPcb()) == NULL)
			printf("allocPcb: unexpected NULL while insert   ");
		switch (i) {
		case 0:
			firstproc = q;
			break;
		case 5:
			midproc = q;
			break;
		case 9:
			lastproc = q;
			break;
		default:
			break;
		}
		insertProcQ(&qa, q);
	}
	printf("inserted 10 elements   \n");
    if (emptyProcQ(qa)) printf("emptyProcQ: unexpected TRUE"   );
    if (headProcQ(qa) != firstproc)
		printf("headProcQ failed   ");
	q = outProcQ(&qa, firstproc);
	if (q == NULL || q != firstproc)
		printf("outProcQ failed on first entry   ");
	freePcb(q);
	q = outProcQ(&qa, midproc);
	if (q == NULL || q != midproc)
		printf("outProcQ failed on middle entry   ");
	freePcb(q);
	if (outProcQ(&qa, procp[0]) != NULL)
		printf("outProcQ failed on nonexistent entry   ");
	printf("outProcQ ok   \n");
    printf("Removing...   \n");
	for (i = 0; i < 8; i++) {
		if ((q = removeProcQ(&qa)) == NULL)
			printf("removeProcQ: unexpected NULL   ");
		freePcb(q);
	}
	if (q != lastproc)
		printf("removeProcQ: failed on last entry   \n");
	if (removeProcQ(&qa) != NULL)
		printf("removeProcQ: removes too many entries \n");

        if (!emptyProcQ(qa))
                printf("emptyProcQ: unexpected FALSE  \n");

	printf("insertProcQ, removeProcQ and emptyProcQ ok   \n");
	printf("process queues module ok      \n");

    printf("checking process trees...\n");

	if (!emptyChild(procp[2]))
	  printf("emptyChild: unexpected FALSE   ");
	

	printf("Inserting...   \n");
	for (i = 1; i < 10; i++) {
		insertChild(procp[0], procp[i]);
	}
	printf("Inserted 9 children   \n");

	if (emptyChild(procp[0]))
	  printf("emptyChild: unexpected TRUE   ");


	q = outChild(procp[1]);
	if (q == NULL || q != procp[1])
		printf("outChild failed on first child   ");
	q = outChild(procp[4]);
	if (q == NULL || q != procp[4])
		printf("outChild failed on middle child   ");
	if (outChild(procp[0]) != NULL)
		printf("outChild failed on nonexistent child   ");
	printf("outChild ok   \n");


	printf("Removing...   \n");
	for (i = 0; i < 7; i++) {
		if ((q = removeChild(procp[0])) == NULL)
			printf("removeChild: unexpected NULL   ");
	}
	if (removeChild(procp[0]) != NULL)
	  printf("removeChild: removes too many children   ");

	if (!emptyChild(procp[0]))
	    printf("emptyChild: unexpected FALSE   ");
	    
	printf("insertChild, removeChild and emptyChild ok   \n");
	printf("process tree module ok      \n");

	for (i = 0; i < 10; i++) 
		freePcb(procp[i]);

    initASL();
	printf("Initialized active semaphore list   \n");

	/* check removeBlocked and insertBlocked */
	printf("insertBlocked test #1 started  \n");
	for (i = 10; i < MAXPROC; i++) {
		procp[i] = allocPcb();
		if (insertBlocked(&sem[i], procp[i]))
			printf("insertBlocked(1): unexpected TRUE   ");
	}
	printf("insertBlocked test #2 started  \n");
	for (i = 0; i < 10; i++) {
		procp[i] = allocPcb();
		if (insertBlocked(&sem[i], procp[i]))
			printf("insertBlocked(2): unexpected TRUE   ");
	}
    
    return 0;
}