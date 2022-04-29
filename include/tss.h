/* tss.h */

struct tss_struct {
	unsigned long tss_pre;
	long tss_esp0;
	unsigned long tss_ss0;
	long tss_esp1;
	unsigned long tss_ss1;
	long tss_esp2;
	unsigned long tss_ss2;
	long tss_pdbr;
	long tss_regs[16];
	long tss_ldt;
	long tss_t_iomap;
};
