#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#include <sys/param.h>

struct utsname {
	char sysname[64];
	char nodename[MAXHOSTNAMELEN];
	char release[64];
	char version[64];
	char machine[64];
};

#endif
