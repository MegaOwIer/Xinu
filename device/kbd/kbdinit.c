/* kbdinit.c - kbdinit */

#include <xinu.h>

struct	kbdcblk	kbdcb;

/*------------------------------------------------------------------------
 *  kbdinit  -  Initialize buffers for a keyboard
 *------------------------------------------------------------------------
 */
devcall	kbdinit()
{
	set_evec(0x21, (uint32)kbddispatch);

	kbdcb.tyihead = kbdcb.tyitail = kbdcb.tyibuff;
	kbdcb.tyisem = semcreate(0);
	kbdcb.tyicursor = 0;

	vgainit();
}
