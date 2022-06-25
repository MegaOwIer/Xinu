/* meminit.c - memory bounds and free list init */

#include <xinu.h>

/* Memory bounds */

void	*minheap;		/* Start of heap			*/
void	*maxheap;		/* Highest valid heap address		*/

/* Memory map structures */

uint32	bootsign = 1;		/* Boot signature of the boot loader	*/

struct	mbootinfo *bootinfo = (struct mbootinfo *)1;
				/* Base address of the multiboot info	*/
				/*  provided by GRUB, initialized just	*/
				/*  to guarantee it is in the DATA	*/
				/*  segment and not the BSS		*/

/* Task state segment */

#define	GDT_ENTRY_TSS		0x7

struct tss_struct TSS = {
	.tss_ss0 = 0x3 << 3
};

/* Segment Descriptor */

struct __attribute__ ((__packed__)) sd {
	unsigned short	sd_lolimit;
	unsigned short	sd_lobase;
	unsigned char	sd_midbase;
	unsigned char   sd_access;
	unsigned char	sd_hilim_fl;
	unsigned char	sd_hibase;
};

#define	NGD			8	/* Number of global descriptor entries	*/
#define FLAGS_GRANULARITY	0x80
#define FLAGS_SIZE		0x40
#define	FLAGS_SETTINGS		(FLAGS_GRANULARITY | FLAGS_SIZE)

struct	sd	gdt_copy[NGD] = {
/*   sd_lolimit  sd_lobase   sd_midbase  sd_access   sd_hilim_fl sd_hibase */
/* 0th entry NULL */
{            0,          0,           0,         0,            0,        0, },
/* 1st, Kernel Code Segment */
{       0xffff,          0,           0,      0x9a,         0xcf,        0, },
/* 2nd, Kernel Data Segment */
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 3rd, Kernel Stack Segment */
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 4th, User Code Segment */
{       0xffff,          0,           0,      0xFA,         0xc0,        0, },
/* 5th, User Data Segment */
{       0xffff,          0,           0,      0xF2,         0xc0,        0, },
/* 6th, User Stack Segment */
{       0xffff,          0,           0,      0xF2,         0xc0,        0, },
/* 7th, TSS descriptor */
[GDT_ENTRY_TSS] = 
{       0xffff,          0,           0,      0x89,         0x00,        0, },
};

extern	struct	sd	gdt[];		/* Global segment table		*/

/* Page table */

uint32	*freelist;
extern	struct	pt	*kpgdir;	/* Kernel page table directory	*/
struct	pt	*kpg[2];		/* Kernel page table		*/

struct	pt	*pgdir = (struct pt *)0x00802000;

/*------------------------------------------------------------------------
 * vminit - initialize virtual memory
 *------------------------------------------------------------------------
 */
void	vminit(void)
{
	struct	mbmregion	*mmap_addr;	/* Ptr to mmap entries		*/
	struct	mbmregion	*mmap_addrend;	/* Ptr to end of mmap region	*/
	uint32 	i, page_start, npage_code;
	uint32	flist_addr;

	mmap_addr = (struct mbmregion*)NULL;
	mmap_addrend = (struct mbmregion*)NULL;

	flist_addr = (uint32)&end + 8192;	/* end + 8KB		*/

	/* Initialize the memory counters		*/
	/*    Heap starts at the (end + 8KB + 8MB)	*/
	minheap = (void*)((uint32)&end + 0x802000);
	maxheap = minheap;

	/* Check if Xinu was loaded using the multiboot specification	*/
	/*   and a memory map was included				*/
	if (bootsign != MULTIBOOT_SIGNATURE) {
		panic("could not find multiboot signature");
	}
	if (!(bootinfo->flags & MULTIBOOT_BOOTINFO_MMAP)) {
		panic("no mmap found in boot info");
	}

	/* Get base address of mmap region (passed by GRUB) */
	mmap_addr = (struct mbmregion*)bootinfo->mmap_addr;

	/* Calculate address that follows the mmap block */
	mmap_addrend = (struct mbmregion*)((uint8*)mmap_addr + bootinfo->mmap_length);

	/* Read mmap blocks and initialize the Xinu free memory list	*/
	while (mmap_addr < mmap_addrend) {

		/* If block is not usable, skip to next block */
		if (mmap_addr->type != MULTIBOOT_MMAP_TYPE_USABLE) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		if ((uint32)maxheap < ((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length)) {
			maxheap = (void*)((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length);
		}

		/* Ignore memory blocks within the Xinu image */
		if ((mmap_addr->base_addr + mmap_addr->length) < ((uint32)minheap)) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		/* The block is usable, so add the pages to Xinu's (virtual) memory list */

		page_start = roundpage(mmap_addr->base_addr);
		while (page_start < truncpage(mmap_addr->base_addr + mmap_addr->length)) {
			uint32 rec_addr = phy2rec(page_start);
			uint32 *rec_phyaddr = (uint32 *)(rec_addr - 0x400000 + flist_addr);
			*rec_phyaddr = (uint32)freelist;
			freelist = (uint32 *)rec_addr;
			page_start += PAGE_SIZE;
		}

		/* Move to the next mmap block */
		mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
	}

	kpgdir = (struct pt *)(flist_addr + 0x400000);	/* end+8KB+4MB	*/
	kpg[0] = kpgdir + 1;
	kpg[1] = kpgdir + 2;

	proctab[NULLPROC].prpgdir = (uint32)kpgdir;

	kpgdir->entry[0] = (uint32)kpg[0] | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;
	kpgdir->entry[1] = (uint32)kpg[1] | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;
	kpgdir->entry[2] = (uint32)kpgdir | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;
	for (i = 3; i < PT_NENTRY; i++) {
		kpgdir->entry[i] = 0;
	}

	/* Initialize kpg[0] */
	npage_code = (uint32)&end / PAGE_SIZE;
	memset((void *)kpg[0]->entry, 0, sizeof(struct pt));
	for (i = 0; i < npage_code; i++) {
		page_start = i << 12;
		if (page_start < (1 << 20)) {	/* I/O mapping		*/
			kpg[0]->entry[i] = page_start | PT_ENTRY_P | PT_ENTRY_W;
		} else if (i < npage_code) {	/* Xinu code and data	*/
			kpg[0]->entry[i] = page_start | PT_ENTRY_P | PT_ENTRY_W | PT_ENTRY_U;
		}
	}
	kpg[0]->entry[i] = (i << 12);
	kpg[0]->entry[i + 1] = ((i + 1) << 12) | PT_ENTRY_P | PT_ENTRY_W;

	/* Initialize kpg[1] */
	for (i = 0; i < PT_NENTRY; i++) {
		page_start = flist_addr + (i << 12);
		kpg[1]->entry[i] = page_start | PT_ENTRY_P | PT_ENTRY_W;
	}

	/* Reinitialize the memory counters 		*/
	/*    to logical address			*/
	minheap = (void *)0x00C00000;
	maxheap = (void *)0xFE000000;
}


/*------------------------------------------------------------------------
 * setsegs  -  Initialize the global segment table and the global TSS
 *------------------------------------------------------------------------
 */
void	setsegs()
{
	extern int	etext;
	struct sd	*psd;
	uint32		np, ds_end;

	ds_end = 0xffffffff/PAGE_SIZE; /* End page number of Data segment */

	psd = &gdt_copy[1];	/* Kernel code segment: identity map from address
				   0 to etext */
	np = ((int)&etext - 0 + PAGE_SIZE-1) / PAGE_SIZE;	/* Number of code pages */
	psd->sd_lolimit = np;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((np >> 16) & 0xff);

	psd = &gdt_copy[2];	/* Kernel data segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[3];	/* Kernel stack segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[4];	/* User code segment */
	psd->sd_lolimit = np;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((np >> 16) & 0xff);

	psd = &gdt_copy[5];	/* User data segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[6];	/* User stack segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[GDT_ENTRY_TSS];	/* TSS descriptor */
	psd->sd_lolimit = sizeof(TSS) - 1;
	psd->sd_lobase = (long)&TSS;
	psd->sd_midbase = ((long)(&TSS) >> 16) & 0xFF;
	psd->sd_hibase = ((long)(&TSS) >> 24) & 0xFF;

	memcpy(gdt, gdt_copy, sizeof(gdt_copy));
}
