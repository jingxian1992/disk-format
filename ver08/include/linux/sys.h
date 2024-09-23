#ifndef _SYS_H_
#define _SYS_H_

extern int sys_fork(void);
extern int sys_write_simple(const char * buf, int count);
extern int sys_pause(void);
extern int sys_print_sys_time(void);
extern int sys_print_sys_info(void);
extern int sys_print_hd_info(int i);
extern int sys_start_shell(void);
extern int sys_setup(void);
extern int sys_ls(int flags);
extern int sys_pwd(void);
extern int sys_chdir(const char * filename);
extern int sys_mkdir(const char * pathname, int mode);
extern int sys_rmdir(const char * name);
extern int sys_sync(void);
extern int sys_format_hd(int index);









#endif
