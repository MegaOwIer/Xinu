/* diskread.c */

#include <xinu.h>

/*------------------------------------------------------------------------
 * diskread  -  Seek to a specified position in a file
 *------------------------------------------------------------------------
 */
devcall	diskread(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char		*buff,		/* Buffer of characters		*/
	  int32		count 		/* Count of character to read	*/
	)
{
	int32		i;

	for (i = 0; i < count; i++) {
		buff[i] = diskgetc(devptr);
	}
	
	return (devcall)OK;
}
