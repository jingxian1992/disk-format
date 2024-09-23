#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include <sys/types.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define CLOCKS_PER_SEC 100

typedef int clock_t;

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

#define	__isleap(year)	\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 1000 == 0))

/* gettimofday returns this */
struct timeval {
	int	tv_sec;		/* seconds */
	int	tv_usec;	/* microseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

struct tms {
	time_t tms_utime;
	time_t tms_stime;
	time_t tms_cutime;
	time_t tms_cstime;
};

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

#define FD_SET(fd,fdsetp)	(*(fdsetp) |= (1 << (fd)))
#define FD_CLR(fd,fdsetp)	(*(fdsetp) &= ~(1 << (fd)))
#define FD_ISSET(fd,fdsetp)	((*(fdsetp) >> fd) & 1)
#define FD_ZERO(fdsetp)		(*(fdsetp) = 0)

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)	\
	((tvp)->tv_sec cmp (uvp)->tv_sec || \
	 (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
#define	timerclear(tvp)		((tvp)->tv_sec = (tvp)->tv_usec = 0)

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

struct utimbuf {
	time_t actime;
	time_t modtime;
};

int gettimeofday(struct timeval * tp, struct timezone * tz);
int select(int width, fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval * timeout);
clock_t clock(void);
time_t time(time_t * tp);
double difftime(time_t time2, time_t time1);
time_t mktime(struct tm * tp);

char * asctime(const struct tm * tp);
char * ctime(const time_t * tp);
struct tm * gmtime(const time_t *tp);
struct tm *localtime(const time_t * tp);
size_t strftime(char * s, size_t smax, const char * fmt, const struct tm * tp);
void tzset(void);
extern time_t times(struct tms * tp);
extern int utime(const char *filename, struct utimbuf *times);

#endif /*_SYS_TIME_H*/
