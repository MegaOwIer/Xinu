/* kbdputc.c - kbdputc */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  kbdputc  -  Write one character to a vga display
 *------------------------------------------------------------------------
 */
devcall	kbdputc(
	  struct dentry *devptr,
	  char		ch
	)
{
        if (ch == TY_NEWLINE) {
                kbdputc(devptr, TY_RETURN);
	}
	vgaputc(ch);
	return OK;
}
