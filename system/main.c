/*  main.c  - main */

#include <xinu.h>

process	main(void)
{
	char	*elf;
	uint32	start;

	elf = getmem(8192);
	memset(elf, 0, 8192);

	read_file("HELLO.ELF", elf, 8192);

	start = get_elf_entrypoint(elf);
	resume(create(elf + start, INITSTK, INITPRIO, "hello", 1, (uint32)printf));

	freemem(elf, 8192);
	
	return OK;
}
