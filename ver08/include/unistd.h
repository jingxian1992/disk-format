#ifndef _UNISTD_H
#define _UNISTD_H

/* ok, this may be a joke, but I'm working on it */
#define _POSIX_VERSION 198808L

#define _POSIX_CHOWN_RESTRICTED	/* only root can do a chown (I think..) */
#define _POSIX_NO_TRUNC		/* no pathname truncation (but see in kernel) */
#define _POSIX_JOB_CONTROL
#define _POSIX_SAVED_IDS	/* Implemented, for whatever good it is */

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE '\0'	/* character to disable things like ^C */
#endif

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#ifndef NULL
#define NULL    ((void *)0)
#endif

/* access */
#define F_OK	0
#define X_OK	1
#define W_OK	2
#define R_OK	4

/* lseek */
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

/* _SC stands for System Configuration. We don't use them much */
#define _SC_ARG_MAX		1
#define _SC_CHILD_MAX		2
#define _SC_CLOCKS_PER_SEC	3
#define _SC_NGROUPS_MAX		4
#define _SC_OPEN_MAX		5
#define _SC_JOB_CONTROL		6
#define _SC_SAVED_IDS		7
#define _SC_VERSION		8

/* more (possibly) configurable things - now pathnames */
#define _PC_LINK_MAX		1
#define _PC_MAX_CANON		2
#define _PC_MAX_INPUT		3
#define _PC_NAME_MAX		4
#define _PC_PATH_MAX		5
#define _PC_PIPE_BUF		6
#define _PC_NO_TRUNC		7
#define _PC_VDISABLE		8
#define _PC_CHOWN_RESTRICTED	9

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/resource.h>

#define __NR_fork		0
#define __NR_write_simple	1
#define __NR_pause		2
#define __NR_print_sys_time		3
#define __NR_print_sys_info		4
#define __NR_print_hd_info		5
#define __NR_start_shell	6
#define __NR_setup			7
#define __NR_ls				8
#define __NR_pwd			9
#define __NR_chdir			10
#define __NR_mkdir			11
#define __NR_rmdir			12
#define __NR_sync			13
#define __NR_format_hd		14

#define _syscall0(type,name) \
type name(void) \
{ \
int __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name)); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall1(type,name,atype,a) \
type name(atype a) \
{ \
int __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((int)(a))); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall2(type,name,atype,a,btype,b) \
type name(atype a,btype b) \
{ \
int __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((int)(a)),"c" ((int)(b))); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a,btype b,ctype c) \
{ \
int __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" ((int)(a)),"c" ((int)(b)),"d" ((int)(c))); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}

extern int errno;

extern int fork(void);
extern int write_simple(const char * buf, int count);
extern int pause(void);
extern int print_sys_time(void);
extern int print_sys_info(void);
extern int print_hd_info(int i);
extern int start_shell(void);
extern int setup(void);
extern int ls(int flags);
extern int pwd(void);
extern int chdir(const char * filename);
extern int mkdir(const char * pathname, int mode);
extern int rmdir(const char * name);
extern int sync(void);
extern int format_hd(int index);






#endif
