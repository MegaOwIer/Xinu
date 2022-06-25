/* create.c - create, newpid */

#include <xinu.h>

local	int newpid();
extern	struct	tss_struct	TSS;

/*------------------------------------------------------------------------
 *  create  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32	create(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*ksaddr;	/* Kernel stack address		*/
	uint32		*usaddr;	/* User stack address		*/
	struct	pt	*new_pgdir, *new_pt0, *old_pt0, *new_ptx;

	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = roundpage(ssize);
	if ((priority < 1) || ((pid=newpid()) == SYSERR)) {
		restore(mask);
		return SYSERR;
	}

	/* Create page table for the new process */
	new_pgdir = (struct pt *)getmem(PAGE_SIZE);
	if (new_pgdir == (struct pt *)SYSERR) {
		restore(mask);
		return SYSERR;
	}
	memset((void *)new_pgdir, 0, PAGE_SIZE);

	new_pt0 = (struct pt *)getmem(PAGE_SIZE);
	if (new_pt0 == (struct pt *)SYSERR) {
		freemem((char *)new_pgdir, PAGE_SIZE);
		restore(mask);
		return SYSERR;
	}
	old_pt0 = (struct pt *)0x00800000;
	for (i = 0; i < PT_NENTRY; i++) {
		new_pt0->entry[i] = old_pt0->entry[i];
	}

	new_ptx = (struct pt *)getmem(PAGE_SIZE);
	if (new_ptx == (struct pt *)SYSERR) {
		freemem((char *)new_pgdir, PAGE_SIZE);
		freemem((char *)new_pt0, PAGE_SIZE);
		restore(mask);
		return SYSERR;
	}
	memset((void *)new_ptx, 0, PAGE_SIZE);

	/* Fill in page table dir */
	new_pgdir->entry[0] = log2phy((char *)new_pt0) | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;
	new_pgdir->entry[1] = pgdir->entry[1];
	new_pgdir->entry[2] = log2phy((char *)new_pgdir) | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;
	new_pgdir->entry[1015] = log2phy((char *)new_ptx) | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;

	/* Allocate stack for the new process, which is on the heap */
	ksaddr = (uint32 *)getstk(KERNELSTK, new_pt0, TRUE);
	if (ksaddr == (uint32 *)SYSERR) {
		freemem((char *)new_pgdir, PAGE_SIZE);
		freemem((char *)new_pt0, PAGE_SIZE);
		freemem((char *)new_ptx, PAGE_SIZE);
		restore(mask);
		return SYSERR;
	}
	usaddr = (uint32 *)getstk(ssize, new_ptx, FALSE);
	if (usaddr == (uint32 *)SYSERR) {
		freemem((char *)new_pgdir, PAGE_SIZE);
		freemem((char *)new_pt0, PAGE_SIZE);
		freemem((char *)new_ptx, PAGE_SIZE);
		freestk((char *)ksaddr, KERNELSTK);
		restore(mask);
		return SYSERR;
	}

	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prkstkbase = (char *)ksaddr;
	prptr->prustkbase = (char *)usaddr;
	prptr->prpgdir = log2phy((char *)new_pgdir);
	prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;


	/* Initialize user stack as if the process were called		*/

	*usaddr = STACKMAGIC;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs - 1;			/* Last argument		*/
	for (i = nargs; i > 0; i--)	/* Machine dependent; copy args	*/
		*--usaddr = *a--;	/* onto created process's stack	*/
	*--usaddr = (long)INITRET;	/* Push on return address	*/

	usaddr = (uint32 *)(USTKBASE - ((uint32)prptr->prustkbase - (uint32)usaddr));

	/* Initialize kernal stack as if the process was called		*/

#define	realkptr(x)		(KSTKBASE - ((uint32)prptr->prkstkbase - (uint32)(x)))

	*ksaddr = STACKMAGIC;
	savsp = realkptr(ksaddr);

	a = (uint32 *)(&nargs + 1);
	a += nargs - 1;
	for (i = nargs; i > 0; i--)
		*--ksaddr = *a--;
	*--ksaddr = (uint32)INITRET;

	prptr->presp0 = (char *)realkptr(ksaddr); /* TSS configuation	*/
	
	/* The fake interrupt return stack				*/
	*--ksaddr = 0x33;		/* SS in user mode		*/
	*--ksaddr = (uint32)usaddr;	/* %esp	in user mode		*/
	*--ksaddr = 0x00000200;		/* eflags			*/
	*--ksaddr = 0x23;		/* CS in user mode		*/
	*--ksaddr = (uint32)funcaddr;
	*--ksaddr = 0x2B;		/* DS in user mode		*/

	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	*--ksaddr = (uint32)ret_k2u;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--ksaddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	savsp = realkptr(ksaddr);	/* Start of frame for ctxsw	*/
	*--ksaddr = 0x00000200;		/* New process runs with	*/
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--ksaddr = 0;			/* %eax */
	*--ksaddr = 0;			/* %ecx */
	*--ksaddr = 0;			/* %edx */
	*--ksaddr = 0;			/* %ebx */
	*--ksaddr = 0;			/* %esp; value filled in below	*/
	pushsp = ksaddr;		/* Remember this location	*/
	*--ksaddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--ksaddr = 0;			/* %esi */
	*--ksaddr = 0;			/* %edi */

	prptr->prstkptr = (char *)realkptr(ksaddr);
	*pushsp = (uint32)prptr->prstkptr;
#undef	realkptr

	fillentry((char *)new_pgdir, 0, 0, TRUE);
	invlpg((void *)new_pgdir);
	fillentry((char *)new_pt0, 0, 0, TRUE);
	invlpg((void *)new_pt0);
	fillentry((char *)new_ptx, 0, 0, TRUE);
	invlpg((void *)new_ptx);

	for (i = 0; i < KERNELSTK; i += PAGE_SIZE) {
		fillentry(prptr->prkstkbase + i, 0, 0, TRUE);
		invlpg((void *)(prptr->prkstkbase + i));
	}
	for (i = 0; i < ssize; i += PAGE_SIZE) {
		fillentry(prptr->prustkbase + i, 0, 0, TRUE);
		invlpg((void *)(prptr->prustkbase + i));
	}
	prptr->prkstkbase = (char *)KSTKBASE;
	prptr->prustkbase = (char *)USTKBASE;

	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}kprintf("newpid error\n");
	return (pid32) SYSERR;
}
