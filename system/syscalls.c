#include <xinu.h>

// Syscall list to dispatch in kernel space

const void *syscalls[] = {
	NULL,
	&create,		// 1
	&resume,		// 2
	&recvclr,		// 3
	&receive,		// 4
	&sleepms,		// 5
	&sleep,			// 6
	&fprintf,		// 7
	&printf,		// 8
	&fscanf,		// 9
	&read,			// 10
	&open,			// 11
	&control,		// 12
	&kill,			// 13
	&getpid,		// 14
	&addargs,		// 15
	&getmem,		// 16
	&freemem,		// 17
	NULL,
};

// Syscall wrapper for doing syscall in user space

uint32 do_syscall(uint32 id, uint32 args_count, ...) {
	uint32 return_value = SYSERR;

	// You may need to pass these veriables to kernel side:

	uint32 *ptr_return_value = &return_value;
	uint32 *args_array = &args_count;

	// Your code here ...
	asm volatile (
		"int	$0x22\n"
		:
		: "d"((long)syscalls[id]), 
		  "c"((long)args_array), 
		  "b"((long)ptr_return_value),
		  "a"((long)proctab[getpid()].presp0)
	);

	return return_value;
}
