#ifndef _SCHED_H
#define _SCHED_H

#define HZ 100
#define EXEC_PAGESIZE 4096

#define NR_TASKS	64
#define TASK_SIZE	0x04000000
#define LIBRARY_SIZE	0x00400000

#if (TASK_SIZE & 0x3fffff)
#error "TASK_SIZE must be multiple of 4M"
#endif

#if (LIBRARY_SIZE & 0x3fffff)
#error "LIBRARY_SIZE must be a multiple of 4M"
#endif

#if (LIBRARY_SIZE >= (TASK_SIZE/2))
#error "LIBRARY_SIZE too damn big!"
#endif

#if (((TASK_SIZE>>16)*NR_TASKS) != 0x10000)
#error "TASK_SIZE*NR_TASKS must be 4GB"
#endif

#define LIBRARY_OFFSET (TASK_SIZE - LIBRARY_SIZE)

#define CT_TO_SECS(x)	((x) / HZ)
#define CT_TO_USECS(x)	(((x) % HZ) * 1000000/HZ)

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE		3
#define TASK_STOPPED		4

#define NGROUPS		32	/* Max number of groups per user */
#define NOGROUP		-1

#define MAXHOSTNAMELEN 8

#ifndef NULL
#define NULL ((void *) 0)
#endif

struct tss_struct {
	int	back_link;	/* 16 high bits zero */
	int	esp0;
	int	ss0;		/* 16 high bits zero */
	int	esp1;
	int	ss1;		/* 16 high bits zero */
	int	esp2;
	int	ss2;		/* 16 high bits zero */
	int	cr3;
	int	eip;
	int	eflags;
	int	eax,ecx,edx,ebx;
	int	esp;
	int	ebp;
	int	esi;
	int	edi;
	int	es;		/* 16 high bits zero */
	int	cs;		/* 16 high bits zero */
	int	ss;		/* 16 high bits zero */
	int	ds;		/* 16 high bits zero */
	int	fs;		/* 16 high bits zero */
	int	gs;		/* 16 high bits zero */
	int	ldt;		/* 16 high bits zero */
	int	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */
};

struct task_struct {
/* these are hardcoded - don't touch */
	int state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	int counter;
	int priority;
	int signal;
	struct sigaction sigaction[32];
	int blocked;	/* bitmap of masked signals */
/* various fields */
	int exit_code;
	unsigned int start_code,end_code,end_data,brk,start_stack;
	int pid,pgrp,session,leader;
	int	groups[NGROUPS];
	/* 
	 * pointers to parent process, youngest child, younger sibling,
	 * older sibling, respectively.  (p->father can be replaced with 
	 * p->p_pptr->pid)
	 */
	struct task_struct	*p_pptr, *p_cptr, *p_ysptr, *p_osptr;
	unsigned short uid,euid,suid;
	unsigned short gid,egid,sgid;
	unsigned int timeout,alarm;
	int utime,stime,cutime,cstime,start_time;
	struct rlimit rlim[RLIM_NLIMITS];
	unsigned int flags;	/* per process flags, defined below */
	unsigned short used_math;
/* file system info */
	int tty;		/* -1 if no tty, so it must be signed */
	unsigned short umask;
	struct m_inode * pwd;
	struct m_inode * root;
	struct m_inode * executable;
	struct m_inode * library;
	unsigned int close_on_exec;
	struct file * filp[NR_OPEN];
/* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
	struct desc_struct ldt[3];
/* tss for this task */
	struct tss_struct tss;
};

extern unsigned int volatile jiffies;
extern unsigned int startup_time;
extern unsigned int jiffies_offset;
extern int hd_timeout;
extern struct task_struct * task[NR_TASKS];
extern struct task_struct * current;
extern struct task_struct * last_task_used_math;
extern int last_pid;

#define CURRENT_TIME (startup_time+(jiffies+jiffies_offset)/HZ)

extern void add_timer(int jiffies, void (*fn)(void));
extern void sleep_on(struct task_struct ** p);
extern void interruptible_sleep_on(struct task_struct ** p);
extern void wake_up(struct task_struct ** p);
extern int in_group_p(gid_t grp);

/*
 * Entry into gdt where to find first TSS. 0-nul, 1-cs, 2-ds, 3-syscall
 * 4-TSS0, 5-LDT0, 6-TSS1 etc ...
 */
#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((((unsigned int) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned int) n)<<4)+(FIRST_LDT_ENTRY<<3))
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))
#define str(n) \
__asm__("str %%ax\n\t" \
	"subl %2,%%eax\n\t" \
	"shrl $4,%%eax" \
	:"=a" (n) \
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3))


/*
 *	switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 * This also clears the TS-flag if the task we switched to has used
 * tha math co-processor latest.
 */
extern void switch_to(int n);

#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)

extern void set_base(void * addr, unsigned int base);
extern void set_limit(void * addr, unsigned int limit);
extern unsigned int get_base(void * addr);
extern unsigned int get_limit(unsigned int segment);

#endif

