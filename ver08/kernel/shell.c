#include <stdio.h>
#include <unistd.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <asm/system.h>

extern int sys_start_shell(void)
{
	int currcons;
	
	for (currcons = 0; currcons < NR_CONSOLES; currcons++)
	{
		change_console(currcons);
		printk(SHELL_INFO);
	}
	change_console(0);
	
	return 100;
}

#define CTYPE_CHAR			1
#define CTYPE_SPACE			2
#define CTYPE_END			3
#define CTYPE_UNDEFINED		100
#define IS_FRONT			1
#define NOT_FRONT			2

static void format_cmd_buf(struct shell_struct * pshell)
{
	int i, j, queue_length;		//i used for shell cmd buf..
	int currcons;				//j used for cycle times counter..
	unsigned int intr_flag;
	struct tty_queue * tmp_queue;
	char ch;
	int char_t_01, char_t_02; 
	
	char_t_01 = CTYPE_UNDEFINED;
	char_t_02 = CTYPE_UNDEFINED;
	i = 0;
	j = 0;
	currcons = fg_console;
	tmp_queue = tty_table[currcons].secondary;
	queue_length = CHARS(tmp_queue);
	
	if (!queue_length)
	{
		pshell->length = 0;
		return;
	}
	else;

	intr_flag = save_intr_flag();
	cli();
	
	while (j < queue_length)
	{
		GETCH(tmp_queue, ch);
		if ((0x20 == ch) || ('\t' == ch))
		{
			j++;
			continue;
		}
		else
		{
			pshell->cmd_buf[i++] = ch;
			j++;
			break;
		}
	}
	
	if (!i)
	{
		pshell->length = 0;
		return;
	}
	else;
	
	while (j < queue_length)
	{
		j++;
		GETCH(tmp_queue, ch);
		if ((0x20 == ch) || ('\t' == ch))
		{
			char_t_02 = CTYPE_SPACE;
		}
		else
		{
			char_t_02 = CTYPE_CHAR;
		}
		
		if (CTYPE_SPACE == char_t_02)
		{
			char_t_01 = char_t_02;
			continue;
		}
		else if (CTYPE_UNDEFINED == char_t_01)
		{
			char_t_01 = char_t_02;
			pshell->cmd_buf[i++] = ch;
		}
		else if (CTYPE_CHAR == char_t_01)
		{
			char_t_01 = char_t_02;
			pshell->cmd_buf[i++] = ch;
		}
		else
		{
			pshell->cmd_buf[i++] = 0x20;
			pshell->cmd_buf[i++] = ch;
		}
		
		char_t_01 = char_t_02;
	}

	pshell->cmd_buf[i] = 0;
	pshell->length = i;
	restore_intr_flag(intr_flag);
	
	return;
}

extern void shell_manage(void)
{
	int currcons;
	char * cmd_buf;
	struct shell_struct * pshell;
	int  tmp;

	currcons = fg_console;
	pshell = shell + currcons;
	cmd_buf = pshell->cmd_buf;
	
	format_cmd_buf(pshell);
	if (!pshell->length)
	{
		goto fast_car;
	}
	else;
		
	if (!strcmp(cmd_buf, "mem") || !strcmp(cmd_buf, "memory"))
	{
		show_mem();
	}
	else if (!strcmp(cmd_buf, "time"))
	{
		print_sys_time();
	}
	else if (!strcmp(cmd_buf, "system") || !strcmp(cmd_buf, "sys"))
	{
		print_sys_info();
	}
	else if (!strcmp(cmd_buf, "task"))
	{
		show_state();
	}
	else if (!strcmp(cmd_buf, "clear") || !strcmp(cmd_buf, "cls"))
	{
		cmd_clear();
		return;
	}
	else if (!strcmp(cmd_buf, "hd0"))
	{
		print_hd_info(0);
	}
	else if (!strcmp(cmd_buf, "hd1"))
	{
		print_hd_info(1);
	}

	else if (!strncmp(cmd_buf, "ls", 2))
	{
		if (!cmd_buf[2])
		{
			ls(0);
		}
		else if (!strcmp(cmd_buf + 2, " -l"))
		{
			ls(1);
		}
		else
		{
			printk("command is supported, and parameter is unknown.");
		}
	}
	else if (!strncmp(cmd_buf, "dir", 3))
	{
		if (!cmd_buf[3])
		{
			ls(0);
		}
		else if (!strcmp(cmd_buf + 3, " -l"))
		{
			ls(1);
		}
		else
		{
			printk("command is supported, and parameter is unknown.");
		}
	}
	else if (!strncmp(cmd_buf, "cd ", 3) && cmd_buf[3])
	{
		chdir(cmd_buf + 3);
	}
	else if (!strncmp(cmd_buf, "chdir ", 6) && cmd_buf[6])
	{
		chdir(cmd_buf + 6);
	}
	else if (!strncmp(cmd_buf, "mkdir ", 6) && cmd_buf[6])
	{
		mkdir(cmd_buf + 6, S_IFDIR | 0777);
	}
	else if (!strncmp(cmd_buf, "rmdir ", 6) && cmd_buf[6])
	{
		rmdir(cmd_buf + 6);
	}
	else if (!strcmp(cmd_buf, "pwd"))
	{
		pwd();
		printk("\n");
	}
	else if (!strcmp(cmd_buf, "fmt-hd"))
	{
		format_hd(1);
	}
	else
	{
		printk("unknown command.\n");
	}

fast_car:	
	printk(SHELL_INFO);
	return;
}

extern void cmd_clear(void)
{
	csi_J(fg_console, 1);
	gotoxy(fg_console, 0, 0);
	set_cursor(fg_console);
	printk(SHELL_INFO);
}


