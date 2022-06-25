/* getheap.c - getheap */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  getheap  -  Find the first unmapped block, return address of the
		first word.
 *------------------------------------------------------------------------
 */
char	*getheap(
	  uint32	nbytes
	)
{
	intmask		mask;
	uint32		npages;
	char 		*addr_begin;
	uint32		page_begin;
	uint32		i;

	mask = disable();
	if (nbytes == 0) {
		restore(mask);
		return (char *)SYSERR;
	}

	nbytes = roundpage(nbytes);		/* Use psize multiples	*/
	npages = nbytes / PAGE_SIZE;

	addr_begin = (char *)0x00C00000;

	/* Find a unmapped block on heap by first-fit */
	while ((uint32)addr_begin < 0xFE000000) {
		for (i = 0; i < npages; i++) {
			page_begin = (uint32)addr_begin + i * PAGE_SIZE;
			if (!isfree((char *)page_begin)) {
				break;
			}
		}

		if (i == npages) {
			restore(mask);
			return addr_begin;
		} else {
			addr_begin += (i + 1) * PAGE_SIZE;
		}
	}

	restore(mask);
	return (char *)SYSERR;
}
