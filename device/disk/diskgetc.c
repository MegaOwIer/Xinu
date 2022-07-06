/* diskgetc.c - diskgetc */

#include <xinu.h>

local	char	lba_read(uint32);

/*------------------------------------------------------------------------
 * diskgetc  -  Read one character from a disk device
 *------------------------------------------------------------------------
 */
devcall	diskgetc(
	  struct dentry	*devptr		/* Entry in device switch table	*/
	)
{
	struct	diskcblk	*diskptr;
	char			ch;

	diskptr = &disktab[devptr->dvminor];
	ch = lba_read(diskptr->lba_addr);
	diskptr->lba_addr++;
	return (devcall)ch;
}


/*------------------------------------------------------------------------
 * lba_read  -  ATA read sectors (LBA mode)
 *------------------------------------------------------------------------
 */
char	lba_read(
	  uint32	addr
	)
{
	static	uint16	cache[256];
	static	uint32	prev_sector = -1;

	uint32	sector, tmp, i;

	sector = addr >> 9;

	if (sector != prev_sector) {
		tmp = (sector >> 24) & 0xF;	/* bit 24-27		*/
		outb(0x01F6, tmp | 0xE0);

		tmp = (sector >> 16) & 0xFF;	/* bit 16-23		*/
		outb(0x01F5, tmp);

		tmp = (sector >> 8) & 0xFF;	/* bit 8-15		*/
		outb(0x01F4, tmp);

		tmp = sector & 0xFF;		/* bit 0-7		*/
		outb(0x01F3, tmp);
		
		outb(0x01F2, 1);		/* number of sectors	*/
		while (inb(0x01F7) & (1 << 7));

		outb(0x01F7, 0x20);		/* Command port		*/

		while (TRUE) {			/* wait until ready	*/
			tmp = inb(0x01F7);
			if (tmp & 8) {
				break;
			}
		}

		insw(0x1F0, (int32)cache, 256);
		prev_sector = sector;
	}

	return *((char *)cache + (addr & 0x1FF));
}
