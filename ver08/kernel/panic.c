#include <stdio.h>
#include <linux/sched.h>

extern volatile void panic_exp(const char * s, const char * file_name, int line_num, const char * func_name)
{
	printk("\tKernel panic: %s\n", s);
	printk("\tfile name: %s\n", file_name);
	printk("\tline number: %d\n", line_num);
	printk("\tfunction name: %s\n", func_name);
	
	if (current == task[0])
		printk("In swapper task - not syncing\n\r");
	
	while(1);
}
