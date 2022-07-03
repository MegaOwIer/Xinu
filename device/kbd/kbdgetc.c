/* kbdgetc.c - kbdgetc */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  kbdgetc  -  Read one character from a keaboard
 *------------------------------------------------------------------------
 */
devcall	kbdgetc(
	  struct dentry	*devptr
	)
{
	char		ch;

	/* Wait for a character in the buffer and extract one character	*/

	wait(kbdcb.tyisem);
	ch = *kbdcb.tyihead++;

	/* Wrap around to beginning of buffer, if needed */

	if (kbdcb.tyihead >= &kbdcb.tyibuff[TY_IBUFLEN]) {
		kbdcb.tyihead = kbdcb.tyibuff;
	}

	return (devcall)ch;
}
