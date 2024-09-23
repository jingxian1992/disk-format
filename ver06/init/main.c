#include <stdio.h>
#include <linux/head.h>
#include <asm/system.h>
#include <time.h>
#include <linux/mm.h>
#include <sys_call.h>
#include <linux/blk.h>

#ifndef RAMDISK
#define RAMDISK 4096
#endif

extern int errno;

extern int rd_init(int mem_start, int length);
extern void mem_init(int start_mem, int end_mem);
extern void blk_dev_init(void);
extern void trap_init(void);
extern void tty_init(void);
extern void timer_init(void);
extern void sched_init(void);
extern void buffer_init(int buffer_end);
extern void hd_init(void);
extern void inode_table_init(void);
extern void super_block_init(void);
extern void file_table_init(void);
extern void system_info_init(void);

void main(int argc, char ** argv, char ** envp)
{
	start_kernel();
	return;
}

static inline _syscall0(int,fork)

void start_kernel(void)
{
	int rd_length;
	int __res;

	errno = 0;
	__res = 0;

	pg_dir = (unsigned int *)0;
	idt = (struct desc_struct *)0xC000;
	gdt = (struct desc_struct *)0xC800;
	
	memory_end = *((short *)0x90002);
	memory_end = memory_end * 1024 + 0x100000;
	
	if (memory_end > (32 * 1024 * 1024))
		memory_end = 32 * 1024 * 1024;
	else;

	if (memory_end > (16 * 1024 *1024))
		buffer_memory_end = 6 * 1024 * 1024;
	else
		buffer_memory_end = 4 * 1024 * 1024;

	main_memory_start = buffer_memory_end;
	memory_end_M = memory_end/(1024 * 1024);	
	buffer_memory_end_M = buffer_memory_end/0x100000;

#ifdef RAMDISK
	rd_length = rd_init(main_memory_start, RAMDISK * 1024);
	main_memory_start += rd_length;
#endif

	mem_init(main_memory_start, memory_end);
	blk_dev_init();
	trap_init();
	tty_init();
	
	if (memory_end < 16 * 0x100000)
		panic("\tthe memory volume is too little.\n");
	else;
	
	time_init();
	sched_init();
	timer_init();
	buffer_init(buffer_memory_end);
	hd_init();	
	inode_table_init();
	super_block_init();
	file_table_init();
	system_info_init();

	sti();
	move_to_user_mode();
	
	__res = fork();

	if (! __res)
	{
		init();			/* we count on this going ok */
	}	
	/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 */
	for(;;)
		__asm__ __volatile__ ("int $0x80"::"a" (__NR_pause));
}

int init(void)
{
	printf("\n\twelcome to dumpling OS.\n");
	printf("\tthis version kernel is used to format virtual hard disk.\n");
	printf("\thd0 is hard codec with parameter in the blk.h, you can modify it to some style that you want.\n");
	printf("\thd1 is decided by your hard disk volume. we allocate nearly the whole disk volume.\n");
	printf("\tformat disk and create root directary need to execute respectively.\n");
	printf("\tfirst step: format disk.\tsecond step: create root directary.\n");
	printf("\tGame Happy @_@\n\n");
	
	start_shell();	

	while (1);
	return 0;
}


