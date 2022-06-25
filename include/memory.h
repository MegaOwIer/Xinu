/* memory.h - roundpage, truncpage, phy2rec, rec2phym, freestk */

#define	PAGE_SIZE	(4096)

/*----------------------------------------------------------------------
 * roundpage, truncpage - Round or truncate address to page size
 *----------------------------------------------------------------------
 */
#define	roundpage(x)	(uint32)((PAGE_SIZE - 1 + (uint32)(x)) & (~(PAGE_SIZE - 1)))
#define	truncpage(x)	(uint32)(((uint32)(x)) & (~(PAGE_SIZE - 1)))

/*----------------------------------------------------------------------
 * phy2rec, rec2phy - Convert between free page addr. and its record
 *----------------------------------------------------------------------
 */
#define phy2rec(x)	(((uint32)(x) / PAGE_SIZE) * 4 + 0x400000)
#define rec2phy(x)	(((uint32)(x) - 0x400000) / 4 * PAGE_SIZE)

/*----------------------------------------------------------------------
 *  freestk  --  Free stack memory allocated by getstk
 *----------------------------------------------------------------------
 */
#define	freestk(p,len)	freemem((char *)((uint32)(p)		\
				- ((uint32)roundpage(len))	\
				+ (uint32)sizeof(uint32)),	\
				(uint32)roundpage(len) )

extern	void	*minheap;		/* Start of heap		*/
extern	void	*maxheap;		/* Highest valid heap address	*/

#define KSTKBASE	((uint32)&end + 2 * PAGE_SIZE - sizeof(uint32))
#define USTKBASE	((uint32)maxheap - sizeof(uint32))

#define	PT_NENTRY	(1 << 10)
#define PT_ENTRY_P	0x1		/* Page table entry present	*/
#define PT_ENTRY_W	0x2		/* Page table entry writable	*/
#define PT_ENTRY_U	0x4		/* Page table entry user	*/
#define	get_addr(x)	((x) & 0xFFFFF000)

struct pt {
	uint32	entry[PT_NENTRY];
};
extern	uint32	*freelist;		/* Head of free page list	*/
extern	struct	pt	*pgdir;

/* Added by linker */

extern	int	text;			/* Start of text segment	*/
extern	int	etext;			/* End of text segment		*/
extern	int	data;			/* Start of data segment	*/
extern	int	edata;			/* End of data segment		*/
extern	int	bss;			/* Start of bss segment		*/
extern	int	ebss;			/* End of bss segment		*/
extern	int	end;			/* End of program		*/
