/* xsh_sort.c - xsh_sort */

#include <xinu.h>

void merge_sort(int32 n, char *args[])
{
	if (n <= 1) {
		return;
	}

	char **buffer = (char **)syscall_getmem(sizeof(char *[n]));

	for (int i = 0; i < n; i++) {
		buffer[i] = args[i];
	}

	merge_sort(n / 2, buffer);
	merge_sort((n + 1) / 2, buffer + n / 2);

	int i = 0, j = 0, now = 0;

	while (i < n / 2 && j < (n + 1) / 2) {
		if (strncmp(buffer[i], buffer[n / 2 + j], PAGE_SIZE) <= 0) {
			args[now++] = buffer[i++];
		} else {
			args[now++] = buffer[n / 2 + j++];
		}
	}

	while (i < n / 2) {
		args[now++] = buffer[i++];
	}

	while (j < (n + 1) / 2) {
		args[now++] = buffer[n / 2 + j++];
	}

	syscall_freemem((char *)buffer, sizeof(char *[n]));
}


/*------------------------------------------------------------------------
 * xsh_sort  -  Shell command to sort args
 *------------------------------------------------------------------------
 */
shellcmd xsh_sort(int32 nargs, char *args[])
{
	if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
		syscall_printf("Use: %s\n\n", args[0]);
		syscall_printf("Description:\n");
		syscall_printf("\tSort args\n");
		syscall_printf("Options:\n");
		syscall_printf("\t--help\t display this help and exit\n");
		return 0;
	}

	merge_sort(nargs - 1, args + 1);

	for (int i = 1; i < nargs; i++) {
		syscall_printf("%s\n", args[i]);
	}

	return 0;
}
