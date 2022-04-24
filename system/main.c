/*  main.c  - main */

#include <xinu.h>

uint32 global_counter = 0;

static const uint32 COUNTER_PER_MS = 250000;

void funcA() {
	while(global_counter < 0xFFFFFFFF) {
		global_counter += 1;
	}
}

void funcB() {
	while(1) {
		while(global_counter < COUNTER_PER_MS * 15) {
			global_counter += 1;
		}
		sleepms(5);
		while(global_counter < COUNTER_PER_MS * 5) {
			global_counter += 1;
		}
		sleepms(5);
	}
}

void funcC() {
	while(global_counter < 0xFFFFFFFF) {
		global_counter += 1;
	}
}

process	main(void)
{
	resume(create(funcA, 8192, PRIO1, "funcA()", 0));
	resume(create(funcB, 8192, PRIO2, "funcB()", 0));
	resume(create(funcC, 8192, PRIO3, "funcC()", 0));
	return OK;
}
