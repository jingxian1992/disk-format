#ifndef _SCHED_H
#define _SCHED_H

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <sys/param.h>
#include <time.h>
#include <sys/resource.h>
#include <signal.h>

#ifndef HZ
#define HZ 100
#endif

#define NR_TASKS	64
#define TASK_SIZE	0x04000000      // 64M per task
#define LIBRARY_SIZE	0x00400000
#define LIBRARY_OFFSET	0x03C00000
#if (TASK_SIZE & 0x3fffff)
#error "TASK_SIZE must be multiple of 4M"
#endif

#ifndef NR_OPEN
#define NR_OPEN 32
#endif

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE		3
#define TASK_STOPPED		4

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

extern unsigned int CURRENT_TIME(void);

extern struct task_struct * task[NR_TASKS];
extern struct task_struct * current;
extern int last_pid;

extern void wake_up(struct task_struct ** p);

#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY 5

extern unsigned int _TSS(int n);
extern unsigned int _LDT(int n);
extern void ltr(int n);
extern void lldt(int n);
extern void save_tr(int * n);
extern void set_base(void * addr, unsigned int base);
extern void set_limit(void * addr, unsigned int limit);	
extern unsigned int get_base(void * addr);
extern unsigned int get_limit(unsigned int segment);

extern int suser(void);
extern unsigned int CT_TO_SECS(unsigned int x);
extern unsigned int CT_TO_USECS(unsigned int x);
extern void hd_times_out(void);
void show_task(int nr,struct task_struct * p);
void show_state(void);

#endif
