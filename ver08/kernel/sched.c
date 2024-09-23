#include <stdio.h>
#include <linux/sched.h>
#include <linux/sys.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <signal.h>
#include <linux/blk.h>

#define _S(nr) (1<<((nr)-1))
#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

extern int timer_interrupt(void);
extern int system_call(void);

unsigned int volatile jiffies;
unsigned int startup_time;
unsigned int jiffies_offset;
struct task_struct * task[NR_TASKS];
struct task_struct * current;
struct task_struct * last_task_used_math;

extern void switch_to(int n)
{
struct {int a,b;} __tmp;
__asm__("cmpl %%ecx,current\n\t"
	"je 1f\n\t"
	"movw %%dx,(%%ebx)\n\t"
	"xchgl %%ecx,current\n\t"
	"ljmp *(%%eax)\n\t"
	"1:"
	::"a" ((void *)(&__tmp.a)),"b" ((void *)(&__tmp.b)),
	"d" (_TSS(n)),"c" ((int) task[n]));
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

#define LATCH (1193180/HZ)

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

#define TIME_REQUESTS 64

struct timer_list {
	int jiffies;
	void (*fn)();
	struct timer_list * next;
} timer_list[TIME_REQUESTS], * next_timer;

void add_timer(int jiffies, void (*fn)(void))
{
	struct timer_list * p;

	if (!fn)
		return;
	cli();
	if (jiffies <= 0)
		(fn)();
	else {
		for (p = timer_list ; p < timer_list + TIME_REQUESTS ; p++)
			if (!p->fn)
				break;
		if (p >= timer_list + TIME_REQUESTS)
			panic("No more time requests free");
		p->fn = fn;
		p->jiffies = jiffies;
		p->next = next_timer;
		next_timer = p;
		while (p->next && p->next->jiffies < p->jiffies) {
			p->jiffies -= p->next->jiffies;
			fn = p->fn;
			p->fn = p->next->fn;
			p->next->fn = fn;
			jiffies = p->jiffies;
			p->jiffies = p->next->jiffies;
			p->next->jiffies = jiffies;
			p = p->next;
		}
	}
	sti();
}

void do_timer(int cpl)
{
	if (hd_timeout)
		if (!--hd_timeout)
			hd_times_out();

	if (cpl)
		current->utime++;
	else
		current->stime++;

	if (next_timer) {
		next_timer->jiffies--;
		while (next_timer && next_timer->jiffies <= 0) {
			void (*fn)(void);
			
			fn = next_timer->fn;
			next_timer->fn = NULL;
			next_timer = next_timer->next;
			(fn)();
		}
	}

	if ((--current->counter)>0) return;
	current->counter=0;
	if (!cpl) return;
	schedule();
}

void sched_init(void)
{
	int i;
	struct desc_struct * p;

	jiffies = 0;
	startup_time = 0;
	jiffies_offset = 0;
	for (i = 0; i < NR_TASKS; i++)
	{
		task[i] = NULL;
	}
	last_task_used_math = NULL;
	last_pid = 0;
	task[0] = (struct task_struct *)0x12000;
	current = task[0];

	if (sizeof(struct sigaction) != 16)
	{
		panic("Struct sigaction MUST be 16 bytes");
	}
	
	for (i = 0; i < TIME_REQUESTS; i++)
	{
		timer_list[i].jiffies = 0;
		timer_list[i].fn = NULL;
		timer_list[i].next = NULL;
	}
	next_timer = NULL;
	
	current->state = 0;
	current->counter = 15;
	current->priority = 15;
	current->signal = 0;
	
	for (i = 0; i < 32; i++)
	{
		current->sigaction[i].sa_handler = NULL;
		current->sigaction[i].sa_mask = 0;
		current->sigaction[i].sa_flags = 0;
		current->sigaction[i].sa_restorer = NULL;
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
		current->groups[i] = NOGROUP;
	}
	
	current->p_pptr = NULL;
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
	
	current->flags = 0;
	current->used_math = 0;
	current->tty = -1;
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
	
	current->ldt[0].a = 0;
	current->ldt[0].b = 0;
	current->ldt[1].a = 0x02FF;
	current->ldt[1].b = 0x00C0FA00;
	current->ldt[2].a = 0x02FF;
	current->ldt[2].b = 0x00C0F200;
	current->tss.back_link = 0;
	current->tss.esp0 = 0x13000;
	current->tss.ss0 = 0x10;
	current->tss.esp1 = 0;
	current->tss.ss1 = 0;
	current->tss.esp2 = 0;
	current->tss.ss2 = 0;
	current->tss.cr3 = (int)pg_dir;
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
	current->tss.cs = 0x0F;
	current->tss.ss = 0x17;
	current->tss.ds = 0x17;
	current->tss.fs = 0x17;
	current->tss.gs = 0x17;
	current->tss.ldt = _LDT(0);
	current->tss.trace_bitmap = 0x80000000;	
	
	set_tss_desc(gdt + FIRST_TSS_ENTRY, &current->tss);
	set_ldt_desc(gdt + FIRST_LDT_ENTRY, current->ldt);
	p = gdt + 2 + FIRST_TSS_ENTRY;
	for(i = 1; i < NR_TASKS; i++) {
		p->a = p->b = 0;
		p++;
		p->a = p->b = 0;
		p++;
	}
/* Clear NT, so that we won't have troubles with that later on */
	__asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");
	ltr(0);
	lldt(0);
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */
	set_intr_gate(0x20, &timer_interrupt);
	outb(inb_p(0x21)&(~0x01), 0x21);
	set_system_gate(0x80, &system_call);
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


