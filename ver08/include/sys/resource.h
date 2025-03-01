/*
 * Resource control/accounting header file for linux
 */

#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H

/*
 * Definition of struct rusage taken from BSD 4.3 Reno
 * 
 * We don't support all of these yet, but we might as well have them....
 * Otherwise, each time we add new items, programs which depend on this
 * structure will lose.  This reduces the chances of that happening.
 */
#define	RUSAGE_SELF	0
#define	RUSAGE_CHILDREN	-1

struct	rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	int	ru_maxrss;		/* maximum resident set size */
	int	ru_ixrss;		/* integral shared memory size */
	int	ru_idrss;		/* integral unshared data size */
	int	ru_isrss;		/* integral unshared stack size */
	int	ru_minflt;		/* page reclaims */
	int	ru_majflt;		/* page faults */
	int	ru_nswap;		/* swaps */
	int	ru_inblock;		/* block input operations */
	int	ru_oublock;		/* block output operations */
	int	ru_msgsnd;		/* messages sent */
	int	ru_msgrcv;		/* messages received */
	int	ru_nsignals;		/* signals received */
	int	ru_nvcsw;		/* voluntary context switches */
	int	ru_nivcsw;		/* involuntary " */
};

/*
 * Resource limits
 */

#define RLIMIT_CPU	0		/* CPU time in ms */
#define RLIMIT_FSIZE	1		/* Maximum filesize */
#define RLIMIT_DATA	2		/* max data size */
#define RLIMIT_STACK	3		/* max stack size */
#define RLIMIT_CORE	4		/* max core file size */
#define RLIMIT_RSS	5		/* max resident set size */

#ifdef notdef
#define RLIMIT_MEMLOCK	6		/* max locked-in-memory address space*/
#define RLIMIT_NPROC	7		/* max number of processes */
#define RLIMIT_OFILE	8		/* max number of open files */
#endif

#define RLIM_NLIMITS	6

#define RLIM_INFINITY	0x7fffffff

struct rlimit {
	int	rlim_cur;
	int	rlim_max;
};

#endif /* _SYS_RESOURCE_H */
