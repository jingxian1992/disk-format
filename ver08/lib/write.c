/*
 *  linux/lib/write.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

//_syscall3(int,write,int,fd,const char *,buf,off_t,count)
extern _syscall0(int, fork)
extern _syscall2(int,write_simple,const char *,buf,int,count)
extern _syscall0(int, pause)
extern _syscall0(int, print_sys_time)
extern _syscall0(int, print_sys_info)
extern _syscall1(int, print_hd_info, int, i)
extern _syscall0(int, start_shell)
extern _syscall0(int, setup)
extern _syscall1(int, ls, int, flags)
extern _syscall0(int, pwd)
extern _syscall1(int, chdir, const char *, filename)
extern _syscall2(int, mkdir, const char *, pathname, int, mode)
extern _syscall1(int, rmdir, const char *, name)
extern _syscall0(int, sync)
extern _syscall1(int, format_hd, int, index)














