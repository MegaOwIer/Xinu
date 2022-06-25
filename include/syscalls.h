#pragma once

#include "valen.h"

#define do_argless_syscall(rettype, id) \
		((rettype)do_syscall(id, 0))
#define do_generic_syscall(rettype, id, ...) \
		((rettype)do_syscall(id, va_args_len(__VA_ARGS__), __VA_ARGS__))

enum SYSCALL_IDX {
	SYSCALL_CREATE	= 1,
	SYSCALL_RESUME	= 2,
	SYSCALL_RECVCLR	= 3,
	SYSCALL_RECEIVE	= 4,
	SYSCALL_SLEEPMS	= 5,
	SYSCALL_SLEEP	= 6,
	SYSCALL_FPRINTF	= 7,
	SYSCALL_PRINTF	= 8,
	SYSCALL_FSCANF	= 9,
	SYSCALL_READ	= 10,
	SYSCALL_OPEN	= 11,
	SYSCALL_CONTROL	= 12,
	SYSCALL_KILL	= 13,
	SYSCALL_GETPID	= 14,
	SYSCALL_ADDARGS	= 15,
	SYSCALL_GETMEM	= 16,
	SYSCALL_FREEMEM	= 17,
};

