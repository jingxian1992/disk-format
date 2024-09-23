/*
 *  linux/kernel/traps.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'Traps.c' handles hardware traps and faults after we have saved some
 * state in 'asm.s'. Currently mostly a debugging-aid, will be extended
 * to mainly kill the offending process (probably by giving it a signal,
 * but possibly by killing it outright if necessary).
 */

#include <stdio.h>
#include <string.h>
#include <linux/head.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/io.h>

static unsigned char get_seg_byte(int seg, char * addr)
{
	char __res;
__asm__("push %%fs;mov %%ax,%%fs;movb %%fs:%2,%%al;pop %%fs"
	:"=a" (__res):"0" (seg),"m" (*(addr)));
	return __res;
}

static unsigned int get_seg_long(int seg, int * addr)
{
	unsigned int __res;
__asm__("push %%fs;mov %%ax,%%fs;movl %%fs:%2,%%eax;pop %%fs"
	:"=a" (__res):"0" (seg),"m" (*(addr)));
	return __res;
}

static unsigned short _fs(void)
{
	unsigned short __res;
__asm__("mov %%fs,%%ax":"=a" (__res):);
	return __res;
}

extern void page_exception(void);

extern void divide_error(void);
extern void debug(void);
extern void nmi(void);
extern void int3(void);
extern void overflow(void);
extern void bounds(void);
extern void invalid_op(void);
extern void device_not_available(void);
extern void double_fault(void);
extern void coprocessor_segment_overrun(void);
extern void invalid_TSS(void);
extern void segment_not_present(void);
extern void stack_segment(void);
extern void general_protection(void);
extern void page_fault(void);
extern void coprocessor_error(void);
extern void reserved(void);
extern void parallel_interrupt(void);
extern void irq13(void);
extern void alignment_check(void);

static void die(char * str,int esp_ptr,int nr)
{
	int * esp = (int *) esp_ptr;
	int i;

	printk("%s: %04x\n\r",str,nr&0xffff);
	printk("EIP:\t%04x:%p\nEFLAGS:\t%p\nESP:\t%04x:%p\n",
		esp[1],esp[0],esp[2],esp[4],esp[3]);
	printk("fs: %04x\n",_fs());
	printk("base: 0X%08X, limit: 0X%X\n",get_base((void *)(&(current->ldt[1]))),get_limit(0x17));
	if (esp[4] == 0x17) {
		printk("Stack: ");
		for (i=0;i<4;i++)
			printk("%p ",get_seg_long(0x17,i+(int *)esp[3]));
		printk("\n");
	}
	str(i);
	printk("Pid: %d, process nr: %d\n\r",current->pid,0xffff & i);
	for(i=0;i<10;i++)
		printk("%02x ",0xff & get_seg_byte(esp[1],(i+(char *)esp[0])));
	printk("\n\r");
//	do_exit(11);		/* play segment exception */
	while(1);
}

void do_double_fault(int esp, int error_code)
{
	die("double fault",esp,error_code);
}

void do_general_protection(int esp, int error_code)
{
	die("general protection",esp,error_code);
}

void do_alignment_check(int esp, int error_code)
{
    die("alignment check",esp,error_code);
}

void do_divide_error(int esp, int error_code)
{
	die("divide error",esp,error_code);
}

void do_int3(int * esp, int error_code,
		int fs,int es,int ds,
		int ebp,int esi,int edi,
		int edx,int ecx,int ebx,int eax)
{
	int tr;

	__asm__("str %%ax":"=a" (tr):"0" (0));
	printk("eax\t\tebx\t\tecx\t\tedx\n\r%8x\t%8x\t%8x\t%8x\n\r",
		eax,ebx,ecx,edx);
	printk("esi\t\tedi\t\tebp\t\tesp\n\r%8x\t%8x\t%8x\t%8x\n\r",
		esi,edi,ebp,(int) esp);
	printk("\n\rds\tes\tfs\ttr\n\r%4x\t%4x\t%4x\t%4x\n\r",
		ds,es,fs,tr);
	printk("EIP: %8x   CS: %4x  EFLAGS: %8x\n\r",esp[0],esp[1],esp[2]);
}

void do_nmi(int esp, int error_code)
{
	die("nmi",esp,error_code);
}

void do_debug(int esp, int error_code)
{
	die("debug",esp,error_code);
}

void do_overflow(int esp, int error_code)
{
	die("overflow",esp,error_code);
}

void do_bounds(int esp, int error_code)
{
	die("bounds",esp,error_code);
}

void do_invalid_op(int esp, int error_code)
{
	die("invalid operand",esp,error_code);
}

void do_device_not_available(int esp, int error_code)
{
	die("device not available",esp,error_code);
}

void do_coprocessor_segment_overrun(int esp, int error_code)
{
	die("coprocessor segment overrun",esp,error_code);
}

void do_invalid_TSS(int esp,int error_code)
{
	die("invalid TSS",esp,error_code);
}

void do_segment_not_present(int esp,int error_code)
{
	die("segment not present",esp,error_code);
}

void do_stack_segment(int esp,int error_code)
{
	die("stack segment",esp,error_code);
}

void do_reserved(int esp, int error_code)
{
	die("reserved (15,17-47) error",esp,error_code);
}

void do_coprocessor_error(int esp,int error_code)
{
	die("coprocessor error", esp, error_code);
}

extern void trap_init(void)
{
	int i;

	set_trap_gate(0,&divide_error);
	set_trap_gate(1,&debug);
	set_trap_gate(2,&nmi);
	set_system_gate(3,&int3);	/* int3-5 can be called from all */
	set_system_gate(4,&overflow);
	set_system_gate(5,&bounds);
	set_trap_gate(6,&invalid_op);
	set_trap_gate(7,&device_not_available);
	set_trap_gate(8,&double_fault);
	set_trap_gate(9,&coprocessor_segment_overrun);
	set_trap_gate(10,&invalid_TSS);
	set_trap_gate(11,&segment_not_present);
	set_trap_gate(12,&stack_segment);
	set_trap_gate(13,&general_protection);
	set_trap_gate(14,&page_fault);
	set_trap_gate(15,&reserved);
	set_trap_gate(16,&coprocessor_error);
	set_trap_gate(17,&alignment_check);
	for (i=18;i<48;i++)
		set_trap_gate(i,&reserved);
	set_trap_gate(45,&irq13);
	outb_p(inb_p(0x21)&0xfb,0x21);
	outb(inb_p(0xA1)&0xdf,0xA1);
	set_trap_gate(39,&parallel_interrupt);
}




