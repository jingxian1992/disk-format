#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#define move_to_user_mode() \
__asm__ ("movl %%esp,%%eax\n\t" \
	"pushl $0x17\n\t" \
	"pushl %%eax\n\t" \
	"pushfl\n\t" \
	"pushl $0x0f\n\t" \
	"pushl $1f\n\t" \
	"iret\n" \
	"1:\tmovl $0x17,%%eax\n\t" \
	"movw %%ax,%%ds\n\t" \
	"movw %%ax,%%es\n\t" \
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:::"ax")

#define flush_steamline_kernel() \
__asm__ __volatile__ ( \
	"ljmp $8, $1f\n\t" \
	"1:":::)

#define flush_steamline_user() \
__asm__ __volatile__ ( \
	"ljmp $15, $1f\n\t" \
	"1:":::)

#define sti() __asm__ ("sti"::)
#define cli() __asm__ ("cli"::)
#define nop() __asm__ ("nop"::)
#define iret() __asm__ ("iret"::)

extern unsigned int save_intr_flag(void);
extern void restore_intr_flag(unsigned int intr_flag);
extern void set_intr_gate(int n, void * addr);
extern void set_trap_gate(int n, void * addr);
extern void set_system_gate(int n, void * addr);
extern void _set_tssldt_desc(void * n, void * addr, int type);
extern void set_tss_desc(void * n, void * addr);
extern void set_ldt_desc(void * n, void * addr);

#endif
