/* kbdread.c - kbdread */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  kbdread  -  Read character(s) from a keyboard device
 *------------------------------------------------------------------------
 */
devcall	kbdread(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char		*buff,		/* Buffer of characters		*/
	  int32		count 		/* Count of character to read	*/
	)
{
	int32	nread;			/* Number of characters read	*/
	int32	firstch;		/* First input character on line*/
	char	ch;			/* Next input character		*/

	if (count < 0) {
		return SYSERR;
	}

	/* Block until input arrives */

	firstch = kbdgetc(devptr);

	/* Check for End-Of-File */

	if (firstch == EOF) {
		return EOF;
	}

	/* Read up to a line */

	ch = (char) firstch;
	*buff++ = ch;
	nread = 1;
	while (nread < count && ch != TY_NEWLINE && ch != TY_RETURN) {
		ch = kbdgetc(devptr);
		*buff++ = ch;
		nread++;
	}
	return nread;
}
