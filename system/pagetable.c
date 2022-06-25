/* pagetable.c - isfree, log2phy and fillentry */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  isfree  -  Check whether a logical page in the heap presents
 *------------------------------------------------------------------------
 */
bool8	isfree(
	  char		*addr		/* Logical addr. of the page	*/
	)
{
	intmask 	mask;    	/* Interrupt mask		*/
	uint32		dir, table;
	struct	pt	*pgtable;

	mask = disable();

	if ((uint32)addr % PAGE_SIZE != 0 || 
	    (uint32)addr < (uint32)minheap || (uint32)addr >= (uint32)maxheap) {
		restore(mask);
		return FALSE;
	}
	
	dir = (uint32)addr >> 22;
	table = ((uint32)addr >> 12) & 0x3FF;
	pgtable = (struct pt *)(0x00800000 | (dir << 12));

	if (~pgdir->entry[dir] & PT_ENTRY_P) {
		restore(mask);
		return TRUE;
	}

	if (~pgtable->entry[table] & PT_ENTRY_P) {
		restore(mask);
		return TRUE;
	}

	restore(mask);
	return FALSE;
}


/*------------------------------------------------------------------------
 *  log2phy  -  Convert logical address to physical address
 *------------------------------------------------------------------------
 */
uint32	log2phy(
	  char		*addr
	)
{
	intmask 	mask;    	/* Interrupt mask		*/
	uint32		dir, table, offset, ret;
	struct	pt	*pgtable;

	mask = disable();

	dir = (uint32)addr >> 22;
	table = ((uint32)addr >> 12) & 0x3FF;
	offset = (uint32)addr & 0xFFF;
	pgtable = (struct pt *)(0x00800000 | (dir << 12));

	if (~pgdir->entry[dir] & PT_ENTRY_P) {
		restore(mask);
		return SYSERR;
	}

	if (~pgtable->entry[table] & PT_ENTRY_P) {
		restore(mask);
		return SYSERR;
	}
	ret = get_addr(pgtable->entry[table]) | offset;

	restore(mask);
	return ret;
}


/*------------------------------------------------------------------------
 *  fillentry  -  Fill in an entry of page table
 *------------------------------------------------------------------------
 */
uint32	fillentry(
	  char			*logical,
	  uint32		physical,
	  uint32		ptmask,
	  bool8			strict
	)
{
	intmask 	mask;    	/* Interrupt mask		*/
	uint32		dir, table;
	uint32		tmp_addr;
	struct	pt	*pgtable;
	uint32		ret;

	mask = disable();

	dir = (uint32)logical >> 22;
	table = ((uint32)logical >> 12) & 0x3FF;
	pgtable = (struct pt *)(0x00800000 | (dir << 12));

	if (~pgdir->entry[dir] & PT_ENTRY_P) {
		if (strict) {
			restore(mask);
			return SYSERR;
		}
		tmp_addr = palloc();
		pgdir->entry[dir] = tmp_addr | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;
		memset((void *)pgtable, 0, PAGE_SIZE);
	}
	ret = get_addr(pgtable->entry[table]);

	if (strict && (~pgtable->entry[table] & PT_ENTRY_P)) {
		restore(mask);
		return SYSERR;
	}

	pgtable->entry[table] = physical | ptmask;

	restore(mask);
	return ret;
}


/*------------------------------------------------------------------------
 *  invlpg  -  Disable a record to a logical address in TLB
 *------------------------------------------------------------------------
 */
void	invlpg(
	  void *va
	)
{
	asm volatile (
		"invlpg (%0)"
		:
		: "r"(va)
		: "memory"
	);
}