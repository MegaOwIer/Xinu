/* resched.c - resched, resched_cntl */

#include <xinu.h>

struct	defer	Defer;

extern int global_counter;

/*------------------------------------------------------------------------
 *  resched  -  Reschedule processor to highest priority eligible process
 *------------------------------------------------------------------------
 */
void	resched(void)		/* Assumes interrupts are disabled	*/
{
	struct procent *ptold;	/* Ptr to table entry for old process	*/
	struct procent *ptnew;	/* Ptr to table entry for new process	*/

	/* If rescheduling is deferred, record attempt and return */

	if (Defer.ndefers > 0) {
		Defer.attempt = TRUE;
		return;
	}

	/* Point to process table entry for the current (old) process */

	ptold = &proctab[currpid];
	if(preempt <= 0) {
		switch(ptold->prclass) {
		case PRIO1:
			ptold->prtimeleft = T1;
			break;
		case PRIO2:
			ptold->prtimeleft = T2;
			break;
		case PRIO3:
			ptold->prtimeleft = T3;
			break;
		default:
			ptold->prtimeleft = QUANTUM;
			break;
		}
	} else {
		ptold->prtimeleft = preempt;
	}

	/* Print and reset value of global_counter */
	// kprintf("%s: %u\n", ptold->prname, global_counter);
	// global_counter = 0;

	// static int resched_count = 0;
	// if (++resched_count > 50) {
	// 	while(1);
	// }

	if (ptold->prstate == PR_CURR) {  /* Process remains eligible */
		/* Old process will no longer remain current */
		ptold->prstate = PR_READY;
		insert(currpid, readylist, ptold->prprio);
	}

	/* Force context switch to highest priority ready process */

	currpid = dequeue(readylist);
	ptnew = &proctab[currpid];
	ptnew->prstate = PR_CURR;
	// preempt = QUANTUM;		/* Reset time slice for process	*/
	preempt = ptnew->prtimeleft;
	ctxsw(&ptold->prstkptr, &ptnew->prstkptr);

	/* Old process returns here when resumed */

	return;
}

/*------------------------------------------------------------------------
 *  resched_cntl  -  Control whether rescheduling is deferred or allowed
 *------------------------------------------------------------------------
 */
status	resched_cntl(		/* Assumes interrupts are disabled	*/
	  int32	defer		/* Either DEFER_START or DEFER_STOP	*/
	)
{
	switch (defer) {

	    case DEFER_START:	/* Handle a deferral request */

		if (Defer.ndefers++ == 0) {
			Defer.attempt = FALSE;
		}
		return OK;

	    case DEFER_STOP:	/* Handle end of deferral */
		if (Defer.ndefers <= 0) {
			return SYSERR;
		}
		if ( (--Defer.ndefers == 0) && Defer.attempt ) {
			resched();
		}
		return OK;

	    default:
		return SYSERR;
	}
}
