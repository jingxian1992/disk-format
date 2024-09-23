/*
 *  linux/fs/file_table.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/fs.h>

struct file file_table[NR_FILE];
unsigned char ACC_MODE_INFO[5];

extern void file_table_init(void)
{
	int i;
	for (i = 0; i < NR_FILE; i++)
	{
		file_table[i].f_mode = 0;
		file_table[i].f_flags = 0;
		file_table[i].f_count = 0;
		file_table[i].f_inode = NULL;
		file_table[i].f_pos = 0;
	}
	
	ACC_MODE_INFO[0] = 004;
	ACC_MODE_INFO[1] = 002;
	ACC_MODE_INFO[2] = 006;
	ACC_MODE_INFO[3] = 0377;
	ACC_MODE_INFO[4] = 0;
}



