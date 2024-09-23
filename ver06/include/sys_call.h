#ifndef _SYS_CALL_H_
#define _SYS_CALL_H_

#define __NR_write_simple 0
#define __NR_print_sys_time 1
#define __NR_print_sys_info 2
#define __NR_pause 3
#define __NR_fork 4
#define __NR_start_shell 5
#define __NR_print_hd_info 6
#define __NR_format_hd 7

extern int errno;

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

/*   the secondry packed function against library function   */
extern int printf(const char * fmt, ...);


/*  user library function that assosiated with system call   */
extern int write_simple(const char * buf, int count);
extern int print_sys_time(void);
extern int print_sys_info(void);
extern int start_shell(void);
extern int print_hd_info(int i);
extern int format_hd(int index);

#endif
