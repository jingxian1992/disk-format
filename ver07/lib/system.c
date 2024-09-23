#include <asm/system.h>
#include <linux/head.h>

extern unsigned int save_intr_flag(void)
{
	unsigned int tmp;
	
__asm__ __volatile__(
	"pushfl\n\t"
	"popl %%eax\n\t"
	"andl $0x0200, %%eax"
	
	:"=a"(tmp):
    );
    return tmp;
}

extern void restore_intr_flag(unsigned int intr_flag)
{
__asm__ __volatile__(
	"pushfl \n\t"
	"popl %%ebx\n\t"
	"orl %%eax, %%ebx\n\t"
	"pushl %%ebx\n\t"
	"popfl"
	::"a"(intr_flag)
    );
}

extern void set_intr_gate(int n, void * addr)
{
	void * gate_addr = (void *)(&(idt[n]));
	unsigned short tmp;
	tmp = (unsigned short)(0x8000 + (0<<13) + (14<<8));
	
	__asm__ __volatile__ (
		"movw %%dx, %%ax\n\t"
		"movw %0, %%dx\n\t"
		"movl %%eax, (%%ebx)\n\t"
		"movl %%edx, 4(%%ebx)"
		:: "c"(tmp), "b"(gate_addr),
		"d"(addr), "a"(0x00080000):
	);
}

extern void set_trap_gate(int n, void * addr)
{
	void * gate_addr = (void *)(&(idt[n]));
	unsigned short tmp;
	tmp = (unsigned short)(0x8000 + (0<<13) + (15<<8));
	
	__asm__ __volatile__ (
		"movw %%dx, %%ax\n\t"
		"movw %0, %%dx\n\t"
		"movl %%eax, (%%ebx)\n\t"
		"movl %%edx, 4(%%ebx)"
		:: "c"(tmp), "b"(gate_addr),
		"d"(addr), "a"(0x00080000):
	);
}

extern void set_system_gate(int n, void * addr)
{
	void * gate_addr = (void *)(&(idt[n]));
	unsigned short tmp;
	tmp = (unsigned short)(0x8000 + (3<<13) + (15<<8));
	
	__asm__ __volatile__ (
		"movw %%dx, %%ax\n\t"
		"movw %0, %%dx\n\t"
		"movl %%eax, (%%ebx)\n\t"
		"movl %%edx, 4(%%ebx)"
		:: "c"(tmp), "b"(gate_addr),
		"d"(addr), "a"(0x00080000):
	);
}


/*
extern inline void set_tss_desc(void * n, void * addr)
{
_set_tssldt_desc(n, addr, 0x89);
}
*/

extern void set_tss_desc(void * n, void * addr)
{
	__asm__ __volatile__ (
		"movw $103, 0(%%ecx)\n\t"
		"movw %%ax, 2(%%ecx)\n\t"
		"rorl $16, %%eax\n\t"
		"movb %%al, 4(%%ecx)\n\t"
		"movb %%dl, 5(%%ecx)\n\t"
		"movb $0x00, 6(%%ecx)\n\t"
		"movb %%ah, 7(%%ecx)\n\t"
		"rorl $16, %%eax"
		::"a"(addr), "c"(n), "d"(0x89)
	);
}

extern void set_ldt_desc(void * n, void * addr)
{
	__asm__ __volatile__ (
		"movw $23, 0(%%ecx)\n\t"
		"movw %%ax, 2(%%ecx)\n\t"
		"rorl $16, %%eax\n\t"
		"movb %%al, 4(%%ecx)\n\t"
		"movb %%dl, 5(%%ecx)\n\t"
		"movb $0x00, 6(%%ecx)\n\t"
		"movb %%ah, 7(%%ecx)\n\t"
		"rorl $16, %%eax"
		::"a"(addr), "c"(n), "d"(0x82)
	);
}



