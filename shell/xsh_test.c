/* xsh_test.c - xsh_test */

#include <xinu.h>

/*------------------------------------------------------------------------
 * xsh_test  -  Shell command to test keyboard driver
 *------------------------------------------------------------------------
 */
shellcmd xsh_test(int32 nargs, char *args[])
{
	char	*cur, *it;

	cur = it = args[1];
	while (*it != '\0') {
		if (*it == '\\') {
			it++;
			if (*it == 'n') {
				*cur++ = '\n';
			} else if (*it == 't') {
				*cur++ = '\t';
			} else {
				syscall_printf("Escape character %-2s not supported\n", it - 1);
				return 0;
			}
		} else {
			*cur++ = *it;
		}
		it++;
	}
	*cur++ = '\0';

	syscall_printf(args[1]);
	syscall_printf("\n");

	return 0;
}
