#include <stdio.h>
#include <linux/sched.h>
#include <string.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <unistd.h>

#ifndef MAJOR_NR
#define MAJOR_NR 3
#endif
#include <linux/blk.h>

#define ORIG_PART_INFO *(unsigned int *)0x901F6
#define ORIG_SWAP_DEV *(unsigned short *)0x901FA
#define ORIG_ROOT_DEV *(unsigned short *)0x901FC

extern void con_init(void);
extern void trap_init(void);
extern void tty_init(void);
extern void chr_dev_init(void);
extern void time_init(void);
extern void inode_table_init(void);
extern void file_table_init(void);
extern void super_block_init(void);
extern void system_info_init(void);
extern void init(void);

void main(void)
{
	void start_kernel(void);
	
	pg_dir = (unsigned int *)0;
	idt = (struct desc_struct *)0x13000;
	gdt = (struct desc_struct *)0x13800;
	start_kernel();
	return;
}

static int memory_end;
static int buffer_memory_end;
static int main_memory_start;

void start_kernel(void)
{
	int i, sum;
	int res;
	
	first_part_sectno = ORIG_PART_INFO;
	first_part_blockno = first_part_sectno / 2;
	SWAP_DEV = ORIG_SWAP_DEV;
	ROOT_DEV = ORIG_ROOT_DEV;
	memory_end = 0x4000000;
	buffer_memory_end = 0xD00000;
	main_memory_start = buffer_memory_end;
	
	con_init();
	mem_init(main_memory_start, memory_end);
	trap_init();
	blk_dev_init();
	tty_init();
	chr_dev_init();
	time_init();
	sched_init();
	buffer_init(buffer_memory_end);
	hd_init();
	inode_table_init();
	file_table_init();
	super_block_init();
	file_table_init();
	system_info_init();
	sti();

	move_to_user_mode();
	
	__asm__ __volatile__ (
	"int $0x80\n\t":"=a"(res):"0"(__NR_fork):);

	if (!res)
	{
		init();
	}

	for(;;)
		__asm__("int $0x80"::"a" (__NR_pause));
}

extern void init(void)
{
	setup();
	
	printf("\tthis system is used to format hard disk.\n");
	printf("\tonly support format second disk.\n");
	printf("\tuse the command \"fmt-hd\" to do it.\n");
	
	start_shell();
	while(1);
}





