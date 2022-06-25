/* kill.c - kill */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  kill  -  Kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
syscall	kill(
	  pid32		pid		/* ID of process to kill	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	uint32	i, j;			/* Index into descriptors	*/
	struct	pt	*cur_pgdir, *cur_pgtable;

	mask = disable();
	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		restore(mask);
		return SYSERR;
	}

	if (--prcount <= 1) {		/* Last user process completes	*/
		xdone();
	}

	send(prptr->prparent, pid);
	for (i=0; i<3; i++) {
		close(prptr->prdesc[i]);
	}

	/* Free stack, heap and page table of this process */
	if (pid == getpid()) {
		/* Heap and user stack */
		for (i = 3; i < PT_NENTRY; i++) {
			if (~pgdir->entry[i] & PT_ENTRY_P) {
				continue;
			}
			cur_pgtable = (struct pt *)(0x00800000 + (i << 12));
			for (j = 0; j < PT_NENTRY; j++) {
				if (cur_pgtable->entry[j] & PT_ENTRY_P) {
					pfree(get_addr(cur_pgtable->entry[j]));
				}
			}
			pfree(get_addr(pgdir->entry[i]));
		}

		/* Kernel stack */
		j = (uint32)&end / PAGE_SIZE + 1;
		cur_pgtable = (struct pt *)0x00800000;
		pfree(get_addr(cur_pgtable->entry[j]));
		pfree(get_addr(pgdir->entry[0]));
	} else {
		cur_pgdir = (struct pt *)getheap(sizeof(struct pt));
		if(cur_pgdir == (struct pt *)SYSERR) {
			panic("Error occured while killing a process.");
			restore(mask);
			return SYSERR;
		}
		fillentry((char *)cur_pgdir, prptr->prpgdir, PT_ENTRY_P | PT_ENTRY_W, FALSE);

		cur_pgtable = (struct pt *)getheap(sizeof(struct pt));
		if(cur_pgtable == (struct pt *)SYSERR) {
			panic("Error occured while killing a process.");
			restore(mask);
			return SYSERR;
		}

		/* Heap and user stack */
		for (i = 3; i < PT_NENTRY; i++) {
			if (~cur_pgdir->entry[i] & PT_ENTRY_P) {
				continue;
			}
			fillentry((char *)cur_pgtable, get_addr(cur_pgdir->entry[i]), 
				  PT_ENTRY_P | PT_ENTRY_W, FALSE);
			for (j = 0; j < PT_NENTRY; j++) {
				if (cur_pgtable->entry[j] & PT_ENTRY_P) {
					pfree(get_addr(cur_pgtable->entry[j]));
				}
			}
			pfree(get_addr(cur_pgdir->entry[i]));
			invlpg((void *)cur_pgtable);
		}

		/* Kernel stack */
		fillentry((char *)cur_pgtable, get_addr(cur_pgdir->entry[0]), 
			  PT_ENTRY_P | PT_ENTRY_W, FALSE);
		j = (uint32)&end / PAGE_SIZE + 1;
		pfree(get_addr(cur_pgtable->entry[j]));
		pfree(get_addr(cur_pgdir->entry[0]));

		fillentry((char *)cur_pgtable, 0, 0, TRUE);
		fillentry((char *)cur_pgdir, 0, 0, TRUE);
	}
	pfree(prptr->prpgdir);

	switch (prptr->prstate) {
	case PR_CURR:
		prptr->prstate = PR_FREE;	/* Suicide */
		resched();

	case PR_SLEEP:
	case PR_RECTIM:
		unsleep(pid);
		prptr->prstate = PR_FREE;
		break;

	case PR_WAIT:
		semtab[prptr->prsem].scount++;
		/* Fall through */

	case PR_READY:
		getitem(pid);		/* Remove from queue */
		/* Fall through */

	default:
		prptr->prstate = PR_FREE;
	}

	restore(mask);
	return OK;
}
