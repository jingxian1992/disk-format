#include <stdio.h>
#include <sys_call.h>
#include <linux/tty.h>
#include <linux/sched.h>

extern _syscall0(int, start_shell)

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

extern void shell_manage(void)
{
	int currcons;
	int i, j, length;
	char ch;
	char * cmd_buf;
	struct tty_queue * queue;
	struct shell_struct * pshell;
	char * str;
	
	currcons = fg_console;
	queue = tty_table[currcons].secondary;
	pshell = shell + currcons;
	cmd_buf = pshell->cmd_buf;
	
	if (EMPTY(queue))
	{
		printk(SHELL_INFO);
		return;
	}
	else
	{
		;
	}
	
	length = CHARS(queue);
	for (i = 0; i < length; i++)
	{
		GETCH(queue, &ch);
		cmd_buf[i] = ch;
	}
	cmd_buf[i] = 0;

	if (!strcmp(cmd_buf, "mem") || !strcmp(cmd_buf, "memory"))
	{
		show_mem();
	}
	else if (!strcmp(cmd_buf, "time"))
	{
		sys_print_sys_time();
	}
	else if (!strcmp(cmd_buf, "system") || !strcmp(cmd_buf, "sys"))
	{
		sys_print_sys_info();
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
	else if (!strncmp(cmd_buf, "fmt-hd ", j = strlen("fmt-hd ")))
	{
		if (!strcmp(cmd_buf + j, "0"))
		{
			format_hd(0);
		}
		else if (!strcmp(cmd_buf + j, "1"))
		{
			format_hd(1);
		}
		else
		{
			panic("hard disk index not support");
		}
	}
	else
	{
		printk("unknown command.\n");
	}
	printk("\n");
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


