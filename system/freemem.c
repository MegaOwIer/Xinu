/* freemem.c - freemem */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  freemem  -  Free a memory block, clear them from page table and 
 	        put the pages back to freelist
 *------------------------------------------------------------------------
 */
syscall	freemem(
	  char		*addr,		/* Pointer to memory block	*/
	  uint32	nbytes		/* Size of block in bytes	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	uint32	npages;			/* # of pages to be free	*/
	char	*page_log;
	uint32	i;

	mask = disable();
	if ((nbytes == 0) || ((uint32) addr < (uint32) minheap)
			  || ((uint32) addr >= (uint32) maxheap)) {
		restore(mask);
		return SYSERR;
	}

	nbytes = roundpage(nbytes);		/* Use psize multiples	*/
	npages = nbytes / PAGE_SIZE;

	for(i = 0; i < npages; i++) {
		page_log = addr + i * PAGE_SIZE;
		pfree(fillentry(page_log, 0, 0, TRUE));
		invlpg((void *)page_log);
	}

	restore(mask);
	return OK;
}
