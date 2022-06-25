/* getstk.c - getstk */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  getstk  -  Allocate stack memory for a new process and add to its 
 	       page table. Return logical address of the highest word
 *------------------------------------------------------------------------
 */
char  	*getstk(
	  uint32		nbytes,		/* Siz of mem requested	*/
	  struct	pt	*pgtable,
	  bool8			is_kernel
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	uint32	npages;			/* # of pages to be allocated	*/
	uint32	page_addr;
	char	*old_log, *new_log;
	uint32	i, table;

	mask = disable();
	if (nbytes == 0 || nbytes > (4 << 20)) {	/* ssize <= 4MB	*/
		restore(mask);
		return (char *)SYSERR;
	}

	nbytes = roundpage(nbytes);		/* Use psize multiples	*/
	npages = nbytes / PAGE_SIZE;

	old_log = getmem(nbytes);

	if (is_kernel) {
		new_log = (char *)&end + 2 * PAGE_SIZE;
	} else {
		new_log = (char *)maxheap;
	}
	new_log -= nbytes;

	for (i = 0; i < npages; i++) {
		table = ((uint32)new_log >> 12) & 0x3FF;
		
		page_addr = log2phy(old_log);
		pgtable->entry[table] = page_addr | PT_ENTRY_P | PT_ENTRY_W;
		if (!is_kernel) {
			pgtable->entry[table] |= PT_ENTRY_U;
		}

		old_log += PAGE_SIZE;
		new_log += PAGE_SIZE;
	}

	restore(mask);
	return old_log - sizeof(uint32);
}
