#include <stdio.h>
#include <linux/sched.h>
#include <sys/sys.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#ifndef HZ
#define HZ 100
#endif

static inline unsigned int _S(int nr)
{
	unsigned int res;
	res = (1<<(nr-1));
	return res;
}
 
#define _BLOCKABLE 0xFFFBFEFF

extern int system_call(void);
extern void timer_interrupt(void);

unsigned int volatile jiffies = 0;
unsigned int startup_time = 0;
unsigned int jiffies_offset = 0;

struct task_struct * task[NR_TASKS];
struct task_struct * current;

static inline switch_to(int n) 
{
	struct {unsigned int a,b;} __tmp;

	void * var_a_p;
	void * var_b_p;
	unsigned int selector;
	unsigned int object_task_addr;
	
	var_a_p = (void *)(&(__tmp.a));
	var_b_p = (void *)(&(__tmp.b));
	selector = _TSS(n);
	object_task_addr = (unsigned int)(task[n]);
	
__asm__ __volatile__("cmpl %%ecx, current\n\t"
	"je 1f\n\t"
	"movw %%dx, (%%edi)\n\t"
	"xchgl %%ecx, current\n\t"
	"ljmp (%%esi)\n\t"
	"1:"
	::"S" (var_a_p), "D" (var_b_p),
	"d" (selector), "c"(object_task_addr):);
}

void show_task(int nr,struct task_struct * p)
{
	int i,j = 4096-sizeof(struct task_struct);

	printk("%d: pid=%d, state=%d, father=%d, child=%d, ",nr,p->pid,
		p->state, p->p_pptr->pid, p->p_cptr ? p->p_cptr->pid : -1);
	i=0;
	while (i<j && !((char *)(p+1))[i])
		i++;
	printk("%d/%d chars free in kstack\n\r",i,j);
	printk("   PC=%08X.", *(1019 + (unsigned int *) p));
	if (p->p_ysptr || p->p_osptr) 
		printk("   Younger sib=%d, older sib=%d\n\r", 
			p->p_ysptr ? p->p_ysptr->pid : -1,
			p->p_osptr ? p->p_osptr->pid : -1);
	else
		printk("\n\r");
}

void show_state(void)
{
	int i;

	printk("\rTask-info:\n\r");
	for (i=0;i<NR_TASKS;i++)
		if (task[i])
			show_task(i,task[i]);
}

/*
 *  'schedule()' is the scheduler function. This is GOOD CODE! There
 * probably won't be any reason to change this, as it should work well
 * in all circumstances (ie gives IO-bound processes good response etc).
 * The one thing you might take a look at is the signal-handler code here.
 *
 *   NOTE!!  Task 0 is the 'idle' task, which gets called when no other
 * tasks can run. It can not be killed, and it cannot sleep. The 'state'
 * information in task[0] is never used.
 */
void schedule(void)
{
	int i,next,c;
	struct task_struct ** p;

/* check alarm, wake up any interruptible tasks that have got a signal */

	for(p = &(task[NR_TASKS-1]) ; p > &(task[0]) ; --p)
		if (*p) {
			if ((*p)->timeout && (*p)->timeout < jiffies) {
				(*p)->timeout = 0;
				if ((*p)->state == TASK_INTERRUPTIBLE)
					(*p)->state = TASK_RUNNING;
			}
			if ((*p)->alarm && (*p)->alarm < jiffies) {
				(*p)->signal |= (1<<(SIGALRM-1));
				(*p)->alarm = 0;
			}
			if (((*p)->signal & ~(_BLOCKABLE & (*p)->blocked)) &&
			(*p)->state==TASK_INTERRUPTIBLE)
				(*p)->state=TASK_RUNNING;
		}

/* this is the scheduler proper: */

	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		while (--i) {
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}
		if (c) break;
		for(p = &(task[NR_TASKS-1]) ; p > &(task[0]) ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}
	switch_to(next);
}

int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

static inline void __sleep_on(struct task_struct **p, int state)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(task[0]))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
	current->state = state;
repeat:	schedule();
	if (*p && *p != current) {
		(**p).state = 0;
		current->state = TASK_UNINTERRUPTIBLE;
		goto repeat;
	}
	if (!*p)
		printk("Warning: *P = NULL\n\r");
	if (*p = tmp)
		tmp->state=0;
}

void interruptible_sleep_on(struct task_struct **p)
{
	__sleep_on(p,TASK_INTERRUPTIBLE);
}

void sleep_on(struct task_struct **p)
{
	__sleep_on(p,TASK_UNINTERRUPTIBLE);
}

void wake_up(struct task_struct **p)
{
	if (p && *p) {
		if ((**p).state == TASK_STOPPED)
			printk("wake_up: TASK_STOPPED");
		if ((**p).state == TASK_ZOMBIE)
			printk("wake_up: TASK_ZOMBIE");
		(**p).state=0;
	}
}

void do_timer(int cpl)
{
	static int blanked = 0;

	if (hd_timeout)
		if (!--hd_timeout)
			hd_times_out();


	if (cpl)
		current->utime++;
	else
		current->stime++;

	if ((--current->counter)>0) return;
	current->counter=0;
	if (!cpl) return;
	schedule();
}

int sys_alarm(int seconds)
{
	int old = current->alarm;

	if (old)
		old = (old - jiffies) / HZ;
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return (old);
}

int sys_getpid(void)
{
	return current->pid;
}

int sys_getppid(void)
{
	return current->p_pptr->pid;
}

int sys_getuid(void)
{
	return current->uid;
}

int sys_geteuid(void)
{
	return current->euid;
}

int sys_getgid(void)
{
	return current->gid;
}

int sys_getegid(void)
{
	return current->egid;
}

int sys_nice(int increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

void timer_init(void)
{
	unsigned int timer_val = 1193180;
	unsigned short LATCH = timer_val/HZ;
	unsigned char low, high;
	unsigned char a, b;

	jiffies = 0;
	startup_time = 0;
	jiffies_offset = 0;

	low = (unsigned char)(LATCH & 0xFF);
	high = (unsigned char)((LATCH>>8) && 0xFF);

	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(low, 0x40);	/* LSB */
	outb(high, 0x40);	/* MSB */
	set_intr_gate(0x20,&timer_interrupt);

//	outb(inb_p(0x21)&~0x01,0x21);
	a = inb_p(0x21);
	b = (unsigned char)(~0x01);
	a = a & b;
	outb_p(a, 0x21);

}

void sched_init(void)
{
	int num_once_used_for_LDT;
	int i;
	struct desc_struct * p;
	
	last_pid = 0;
	num_once_used_for_LDT = _LDT(0);

	for (i = 0; i < NR_TASKS; i++)
	{
		task[i] = NULL;
	}

	current = (struct task_struct *)0xA000;
	task[0] = current;
	
	current->state = 0;
	current->counter = 15;
	current->priority = 15;
	
	current->signal = 0;
	for (i = 0; i < 32; i++)
	{
		current->sigaction[i].sa_handler = SIG_DFL;
		current->sigaction[i].sa_mask = 0;
		current->sigaction[i].sa_flags = 0;
		current->sigaction[i].sa_restorer = SIG_DFL;
	}
	current->blocked = 0;
	
	current->exit_code = 0;
	current->start_code = 0;
	current->end_code = 0;
	current->end_data = 0;
	current->brk = 0;
	current->start_stack = 0;
	
	current->pid = 0;
	current->pgrp = 0;
	current->session = 0;
	current->leader = 0;

	for (i = 0; i < NGROUPS; i++)
	{
		current->groups[i] = -1;
	}

	current->p_pptr = current;
	current->p_cptr = NULL;
	current->p_ysptr = NULL;
	current->p_osptr = NULL;
	
	current->uid = 0;
	current->euid = 0;
	current->suid = 0;
	current->gid = 0;
	current->egid = 0;
	current->sgid = 0;
	
	current->timeout = 0;
	current->alarm = 0;
	current->utime = 0;
	current->stime = 0;
	current->cutime = 0;
	current->cstime = 0;
	current->start_time = 0;
	
	for (i = 0; i < RLIM_NLIMITS; i++)
	{
		current->rlim[i].rlim_cur = RLIM_INFINITY;
		current->rlim[i].rlim_max = RLIM_INFINITY;
	}
	
	current->tty = -1;		/* -1 if no tty, so it must be signed */
	current->umask = 0022;
	current->pwd = NULL;
	current->root = NULL;
	current->executable = NULL;
	current->library = NULL;
	current->close_on_exec = 0;
	
	for (i = 0; i < NR_OPEN; i++)
	{
		current->filp[i] = NULL;
	}
	
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	current->ldt[0].low4bit = 0;
	current->ldt[0].high4bit = 0;
	current->ldt[1].low4bit = 0x9f;
	current->ldt[1].high4bit = 0xc0fa00;
	current->ldt[2].low4bit = 0x9f;
	current->ldt[2].high4bit = 0xc0f200;
	
	current->tss.back_link = 0;
	current->tss.esp0 = 0xB000;
	current->tss.ss0 = 0x10;
	current->tss.esp1 = 0;
	current->tss.ss1 = 0;
	current->tss.esp2 = 0;
	current->tss.ss2 = 0;
	current->tss.cr3 = 0;
	current->tss.eip = 0;
	current->tss.eflags = 0;
	current->tss.eax = 0;
	current->tss.ecx = 0;
	current->tss.edx = 0;
	current->tss.ebx = 0;
	current->tss.esp = 0;
	current->tss.ebp = 0;
	current->tss.esi = 0;
	current->tss.edi = 0;
	current->tss.es = 0x17;
	current->tss.cs = 0x0f;
	current->tss.ss = 0x17;
	current->tss.ds = 0x17;
	current->tss.fs = 0x17;
	current->tss.gs = 0x17;
	current->tss.ldt = num_once_used_for_LDT;
	current->tss.trace_bitmap = 0x80000000;	/* bits: trace 0, bitmap 16-31 */
	
	if (sizeof(struct sigaction) != 16)
		panic("Struct sigaction MUST be 16 bytes");

	set_tss_desc((void *)(gdt + FIRST_TSS_ENTRY), (void *)(&(current->tss)));
	set_ldt_desc((void *)(gdt + FIRST_LDT_ENTRY), (void *)(&(current->ldt)));

	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {
		task[i] = NULL;
		p->low4bit=p->high4bit=0;
		p++;
		p->low4bit=p->high4bit=0;
		p++;
	}
/* Clear NT, so that we won't have troubles with that later on */
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
	
	ltr(0);
	lldt(0);
	
	set_system_gate(0x80, system_call);
}

unsigned int _TSS(int n)
{
	unsigned int num;
	num = (unsigned int)n;
	num = num << 4;
	num += (FIRST_TSS_ENTRY << 3);
	return num;
}

unsigned int _LDT(int n)
{
	unsigned int num;
	num = (unsigned int)n;
	num = num << 4;
	num += (FIRST_LDT_ENTRY<<3);
	return num;
}

void ltr(int n)
{
	unsigned int num;
	num = _TSS(n);
__asm__ __volatile__("ltr %%ax"::"a"(num):);
}

void lldt(int n)
{
	unsigned int num;
	num = _LDT(n);
__asm__ __volatile__("lldt %%ax"::"a"(num):);
}

void save_tr(int * n)
{
	int num;
__asm__("str %%ax\n\t"
	"subl %2,%%eax\n\t"
	"shrl $4,%%eax"
	:"=a" (num)
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3));
	
	*n = num;
	return;
}


void set_base(void * addr, unsigned int base)
{
__asm__ __volatile__(
	"movw %%dx, 2(%%ebx)\n\t"
	"rorl $16, %%edx\n\t"
	"movb %%dl,4(%%ebx)\n\t"
	"movb %%dh,7(%%ebx)"
	::"b"(addr), "d"(base):);
}

void set_limit(void * addr, unsigned int limit)
{
	limit = (limit - 1) >> 12;
__asm__ __volatile__(
	"movw %%dx, (%%ebx)\n\t"
	"rorl $16, %%edx\n\t"
	"movb 6(%%ebx), %%dh\n\t"
	"andb $0xf0, %%dh\n\t"
	"orb %%dh, %%dl\n\t"
	"movb %%dl, 6(%%ebx)"
	::"b"(addr), "d"(limit):);
}	

unsigned int get_base(void * addr)
{
	unsigned int __base;
__asm__("movb 7(%%ebx), %%dh\n\t"
	"movb 4(%%ebx), %%dl\n\t"
	"shll $16, %%edx\n\t"
	"movw 2(%%ebx), %%dx"
	:"=d"(__base)
	:"b"(addr):);
	return __base;	
}

unsigned int get_limit(unsigned int segment)
{
	unsigned int __limit;
__asm__("lsll %1,%0\n\tincl %0":"=a" (__limit):"d" (segment));
	return __limit;
}

extern unsigned int CURRENT_TIME(void)
{
	unsigned int res;
	res = startup_time + (jiffies + jiffies_offset)/HZ;
	return res;
}

extern int suser(void)
{
	if (current->euid == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

extern unsigned int CT_TO_SECS(unsigned int x)
{
	unsigned int res;
	res = x / HZ;
	return res;
}

extern unsigned int CT_TO_USECS(unsigned int x)
{
	unsigned int res;
	res = (x % HZ) * 1000000 / HZ;
	return res;
}













