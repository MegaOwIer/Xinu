/* addargs.c - addargs */

#include <xinu.h>
#include "shprototypes.h"


status	upd_stk(
	  uint32	*argloc,	/* Lowest location in the	*/
	  				/*   process stack where the	*/
					/*   args array will be stored	*/
	  uint32	*stk_end,
	  char		*stk_base,
	  int32		ntok,		/* Count of arguments		*/
	  int32		tok[],		/* Index of tokens in tokbuf	*/
	  int32		tlen,		/* Length of data in tokbuf	*/
	  char		*tokbuf,	/* Array of null-term. tokens	*/
	  void 		*dummy		/* Dummy argument that was	*/
					/*   used at creation and must	*/
					/*   be replaced by a pointer	*/
					/*   to an argument vector	*/
	)
{
	char	*argstr;		/* Location in process's stack	*/
					/*   to place arg strings	*/
	uint32	*real_loc;
	uint32	*search;		/* pointer that searches for	*/
					/*   dummy argument on stack	*/
	uint32	*aptr;			/* Walks through args array	*/
	int32	i;			/* Index into tok array		*/

	/* Compute the first location beyond args array for the strings	*/

	real_loc = (uint32 *)(stk_base + 4 - ((uint32)stk_end - (uint32)argloc));

	argstr = (char *) (real_loc + (ntok+1)); /* +1 for a null ptr	*/

	/* Set each location in the args vector to be the address of	*/
	/*	string area plus the offset of this argument		*/

	for (aptr=argloc, i=0; i < ntok; i++) {
		*aptr++ = (uint32) (argstr + tok[i]);
	}

	/* Add a null pointer to the args array */

	*aptr++ = (uint32)NULL;

	/* Copy the argument strings from tokbuf into process's	stack	*/
	/*	just beyond the args vector				*/

	memcpy(aptr, tokbuf, tlen);

	/* Find the second argument in process's stack */

	for (search = (uint32 *)argloc; search < (uint32 *)stk_end; search++) {

		/* If found, replace with the address of the args vector*/
		if (*search == (uint32)dummy) {
			*search = (uint32)real_loc;
			return OK;
		}
	}

	/* Argument value not found on the stack - report an error */
	return SYSERR;
}


/*------------------------------------------------------------------------
 *  addargs  -  Add local copy of argv-style arguments to the stack of
 *		  a command process that has been created by the shell
 *------------------------------------------------------------------------
 */
status	addargs(
	  pid32		pid,		/* ID of process to use		*/
	  int32		ntok,		/* Count of arguments		*/
	  int32		tok[],		/* Index of tokens in tokbuf	*/
	  int32		tlen,		/* Length of data in tokbuf	*/
	  char		*tokbuf,	/* Array of null-term. tokens	*/
	  void 		*dummy		/* Dummy argument that was	*/
					/*   used at creation and must	*/
					/*   be replaced by a pointer	*/
					/*   to an argument vector	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process' table entry	*/
	struct	pt	*pgdir, *pgtable;
	char	*kstk, *ustk;
	bool8	error;

	mask = disable();
	error = FALSE;

	/* Check argument count and data length */

	if ( (ntok <= 0) || (tlen < 0) ) {
		restore(mask);
		return SYSERR;
	}

	prptr = &proctab[pid];

	/* Map stack of the new process to heap */
	pgdir = (struct pt *)getheap(sizeof(struct pt));
	if (pgdir == (struct pt *)SYSERR) {
		restore(mask);
		return SYSERR;
	}
	fillentry((char *)pgdir, prptr->prpgdir, PT_ENTRY_P | PT_ENTRY_W, FALSE);

	pgtable = (struct pt *)getheap(sizeof(struct pt));
	if (pgtable == (struct pt *)SYSERR) {
		fillentry((char *)pgdir, 0, 0, TRUE);
		invlpg((void *)pgdir);
		restore(mask);
		return SYSERR;
	}
	fillentry((char *)pgtable, get_addr(pgdir->entry[0]), PT_ENTRY_P | PT_ENTRY_W, FALSE);

	kstk = getheap(KERNELSTK);
	if (kstk == (char *)SYSERR) {
		fillentry((char *)pgdir, 0, 0, TRUE);
		invlpg((void *)pgdir);
		fillentry((char *)pgtable, 0, 0, TRUE);
		invlpg((void *)pgtable);
		restore(mask);
		return SYSERR;
	}
	fillentry(kstk, get_addr(pgtable->entry[1 + (uint32)&end / PAGE_SIZE]), PT_ENTRY_P | PT_ENTRY_W, FALSE);

	fillentry((char *)pgtable, 0, 0, FALSE);
	invlpg((void *)pgtable);


	fillentry((char *)pgtable, get_addr(pgdir->entry[1015]), PT_ENTRY_P | PT_ENTRY_W, FALSE);
	ustk = getheap(prptr->prstklen);
	if (ustk == (char *)SYSERR) {
		fillentry((char *)pgdir, 0, 0, TRUE);
		invlpg((void *)pgdir);
		fillentry((char *)pgtable, 0, 0, TRUE);
		invlpg((void *)pgtable);
		fillentry(kstk, 0, 0, TRUE);
		invlpg((void *)kstk);
		restore(mask);
		return SYSERR;
	}
	fillentry(ustk, get_addr(pgtable->entry[1023]), PT_ENTRY_P | PT_ENTRY_W, FALSE);
	fillentry((char *)pgtable, 0, 0, FALSE);
	invlpg((void *)pgtable);

	if (upd_stk((uint32 *)ustk, (uint32 *)(ustk + prptr->prstklen), 
		    prptr->prustkbase, ntok, tok, tlen, tokbuf, dummy) == SYSERR) {
			error = TRUE;
	}

	if (upd_stk((uint32 *)kstk, (uint32 *)(kstk + KERNELSTK), 
		    prptr->prkstkbase, ntok, tok, tlen, tokbuf, dummy) == SYSERR) {
			error = TRUE;
	}

	fillentry(kstk, 0, 0, FALSE);
	invlpg((void *)kstk);
	fillentry(ustk, 0, 0, FALSE);
	invlpg((void *)ustk);
	fillentry((char *)pgdir, 0, 0, FALSE);
	invlpg((void *)pgdir);

	restore(mask);
	return error ? SYSERR : OK;
}
