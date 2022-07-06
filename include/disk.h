/* disk.c */

struct	diskcblk {
	uint32	lba_addr;
};

extern	struct	diskcblk	disktab[];
