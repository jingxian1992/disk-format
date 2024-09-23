/*
 *  linux/fs/file_table.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <linux/sched.h>

struct file file_table[NR_FILE];
char acc_mode_str[5];

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
	
	acc_mode_str[0] = 004;
	acc_mode_str[1] = 002;
	acc_mode_str[2] = 006;
	acc_mode_str[3] = 377;
	acc_mode_str[4] = 0;
}

extern int IS_SEEKABLE(int x)
{
	if (x >= 1 && x <= 3)
		return 1;
	else
		return 0;
}

extern int MAJOR(int a)
{
	int major_dev;
	major_dev = (((unsigned int)a) >> 8);
	return major_dev;
}

extern int MINOR(int a)
{
	int minor_dev;
	minor_dev = a & 0xFF;
	return minor_dev;
}

struct task_struct * PIPE_READ_WAIT(struct m_inode * inode)
{
	struct task_struct * res;
	res = inode->i_wait;
	return res;
}

struct task_struct * PIPE_WRITE_WAIT(struct m_inode * inode)
{
	struct task_struct * res;
	res = inode->i_wait2;
	return res;
}

unsigned short PIPE_HEAD(struct m_inode * inode)
{
	unsigned short res;
	res = inode->i_zone[0];
	return res;
}

unsigned short PIPE_TAIL(struct m_inode * inode)
{
	unsigned short res;
	res = inode->i_zone[1];
	return res;
}

int PIPE_SIZE(struct m_inode * inode)
{
	unsigned short head, tail;
	int res;
	head = inode->i_zone[0];
	tail = inode->i_zone[1];
	res = (int)(head - tail) & (PAGE_SIZE - 1);
	return res;
}

int PIPE_EMPTY(struct m_inode * inode)
{
	unsigned short head, tail;
	int res;
	head = inode->i_zone[0];
	tail = inode->i_zone[1];
	if (head == tail)
		res = 1;
	else
		res = 0;

	return res;

}

int PIPE_FULL(struct m_inode * inode)
{
	unsigned short head, tail;
	int res, size;
	head = inode->i_zone[0];
	tail = inode->i_zone[1];
	size = (int)(head - tail) & (PAGE_SIZE - 1);
	
	if ((PAGE_SIZE - 1) == size)
		res = 1;
	else
		res = 0;
	
	return res;

}

extern int S_ISLNK(unsigned short m)
{
	int res;
	res = (((m) & S_IFMT) == S_IFLNK);
	return res;
}

extern int S_ISREG(unsigned short m)
{
	int res;
	res = (((m) & S_IFMT) == S_IFREG);
	return res;
}
	
extern int S_ISDIR(unsigned short m)
{
	int res;
	res = (((m) & S_IFMT) == S_IFDIR);
	return res;
}

extern int S_ISCHR(unsigned short m)
{
	int res;
	res = (((m) & S_IFMT) == S_IFCHR);
	return res;
}
	
extern int S_ISBLK(unsigned short m)
{
	int res;
	res = (((m) & S_IFMT) == S_IFBLK);
	return res;
}

extern int S_ISFIFO(unsigned short m)
{
	int res;
	res = (((m) & S_IFMT) == S_IFIFO);
	return res;
}


