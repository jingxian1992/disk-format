#ifndef _SYS_H_
#define _SYS_H_

extern int sys_write_simple(const char * user_buf, int count);
extern int sys_print_sys_time(void);
extern int sys_print_sys_info(void);
extern int sys_print_hd_info(int i);
extern int sys_format_hd(int index);

extern int sys_alarm(int seconds);
extern int sys_getpid(void);
extern int sys_getppid(void);
extern int sys_getuid(void);
extern int sys_geteuid(void);
extern int sys_getgid(void);
extern int sys_getegid(void);
extern int sys_nice(int increment);
extern int sys_pause(void);
extern int sys_ftime();
extern int sys_break();
extern int sys_ptrace();
extern int sys_stty();
extern int sys_gtty();
extern int sys_rename();
extern int sys_prof();
extern int sys_setregid(int rgid, int egid);
extern int sys_setgid(int gid);
extern int sys_acct();
extern int sys_phys();
extern int sys_lock();
extern int sys_mpx();
extern int sys_ulimit();
extern int sys_time(int * tloc);
extern int sys_setreuid(int ruid, int euid);
extern int sys_setuid(int uid);
extern int sys_stime(int * tptr);
extern int sys_times(struct tms * tbuf);
extern int sys_brk(unsigned int end_data_seg);
extern int sys_setpgid(int pid, int pgid);
extern int sys_getpgrp(void);
extern int sys_setsid(void);
extern int sys_getgroups(int gidsetsize, unsigned short *grouplist);
extern int sys_setgroups(int gidsetsize, unsigned short *grouplist);
extern int in_group_p(unsigned short grp);
extern int sys_uname(struct utsname * name);
extern int sys_sethostname(char *name, int len);
extern int sys_getrlimit(int resource, struct rlimit *rlim);
extern int sys_setrlimit(int resource, struct rlimit *rlim);
extern int sys_getrusage(int who, struct rusage *ru);
extern int sys_gettimeofday(struct timeval *tv, struct timezone *tz);
extern int sys_settimeofday(struct timeval *tv, struct timezone *tz);
extern int sys_umask(int mask);
extern int sys_fork(void);
extern int sys_close(unsigned int fd);



#endif
