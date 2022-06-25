/* getmem.c - getmem */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  getmem  -  Allocate heap storage, map to page table, and
 	       return logical address of the lowest word
 *------------------------------------------------------------------------
 */
char  	*getmem(
	  uint32	nbytes			/* Size of memory requested	*/
	)
{
	intmask		mask;			/* Saved interrupt mask		*/
	uint32		npages;			/* # of pages to be allocated	*/
	char		*page_log, *mem_begin;
	uint32		page_phy;
	uint32		i;

	mask = disable();
	if (nbytes == 0) {
		restore(mask);
		return (char *)SYSERR;
	}

	nbytes = roundpage(nbytes);		/* Use psize multiples	*/
	npages = nbytes / PAGE_SIZE;
	
	mem_begin = getheap(nbytes);

	/* Allocating pages and mapping */
	if (mem_begin != (char *)SYSERR) {
		for (i = 0; i < npages; i++) {
			page_log = mem_begin + i * PAGE_SIZE;
			page_phy = palloc();
			fillentry(page_log, page_phy, PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U, FALSE);
		}
	}

	restore(mask);
	return mem_begin;
}
