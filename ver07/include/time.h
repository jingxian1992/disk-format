#ifndef _TIME_H
#define _TIME_H

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define CLOCKS_PER_SEC 100

#define	DST_NONE	0	/* not on dst */
#define	DST_USA		1	/* USA style dst */
#define	DST_AUST	2	/* Australian style dst */
#define	DST_WET		3	/* Western European dst */
#define	DST_MET		4	/* Middle European dst */
#define	DST_EET		5	/* Eastern European dst */
#define	DST_CAN		6	/* Canada */
#define	DST_GB		7	/* Great Britain and Eire */
#define	DST_RUM		8	/* Rumania */
#define	DST_TUR		9	/* Turkey */
#define	DST_AUSTALT	10	/* Australian style with shift in 1986 */

/* gettimofday returns this */
struct timeval {
	int	tv_sec;		/* seconds */
	int	tv_usec;	/* microseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

extern unsigned int startup_time;

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

struct utimbuf {
	unsigned int actime;
	unsigned int modtime;
};


extern void time_init(void);
extern int CMOS_READ(unsigned char addr);
extern void BCD_TO_BIN(int * val_ptr);
extern unsigned int kernel_mktime(struct tm * tm);
extern int utime(const char *filename, struct utimbuf *times);

extern void FD_SET(int fd, unsigned int * fdsetp);
extern void FD_CLR(int fd, unsigned int * fdsetp);
extern int FD_ISSET(int fd, unsigned int * fdsetp);
extern void FD_ZERO(unsigned int * fdsetp);

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
extern int	timerisset(struct timeval * tvp);
extern int timercmp(struct timeval * tvp, struct timeval * uvp);
extern void timerclear(struct timeval * tvp);

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
#define	ITIMER_REAL	0
#define	ITIMER_VIRTUAL	1
#define	ITIMER_PROF	2

struct	itimerval {
	struct	timeval it_interval;	/* timer interval */
	struct	timeval it_value;	/* current value */
};

struct tms {
	unsigned int tms_utime;
	unsigned int tms_stime;
	unsigned int tms_cutime;
	unsigned int tms_cstime;
};

extern int gettimeofday(struct timeval * tp, struct timezone * tz);

#endif
