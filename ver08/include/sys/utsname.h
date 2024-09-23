#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#include <sys/types.h>
#include <linux/sched.h>

struct utsname {
	char sysname[64];
	char nodename[64];
	char release[64];
	char version[64];
	char * machine[64];
};
struct utsname thisname;
//UTS_SYSNAME, UTS_NODENAME, UTS_RELEASE, UTS_VERSION, UTS_MACHINE

extern int uname(struct utsname * utsbuf);

#endif
