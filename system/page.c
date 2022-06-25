/* page.c - palloc and pfree */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  palloc  -  Allocate a new page, return physical address of 
 	       the lowest word
 *------------------------------------------------------------------------
 */
uint32	palloc()
{
	intmask 	mask;    	/* Interrupt mask		*/
	uint32		ret;

	mask = disable();
	ret = rec2phy(freelist);	/* Get the first free page	*/

	if(ret == 0) {
		panic("[Error] No free page available.");
		return SYSERR;
	}

	freelist = (uint32 *)*freelist;
	restore(mask);
	return ret;
}


/*------------------------------------------------------------------------
 *  pfree  -  Free a page, returning the block to the free list
 *------------------------------------------------------------------------
 */
syscall	pfree(
	  uint32	phy_addr	/* Physical addr. of the page	*/
	)
{
	intmask 	mask;    	/* Interrupt mask		*/
	uint32		*rec_addr;	/* Addr. of the record.		*/

	mask = disable();

	if (phy_addr % PAGE_SIZE != 0 || 
	    phy_addr < (uint32)minheap || phy_addr >= (uint32)maxheap) {
		restore(mask);
		return SYSERR;
	}
	
	rec_addr = (uint32 *)phy2rec(phy_addr);;
	*rec_addr = (uint32)freelist;
	freelist = rec_addr;

	restore(mask);
	return OK;
}
