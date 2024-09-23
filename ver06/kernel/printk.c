#include <stdio.h>
#include <sys/sys.h>

char printk_buf[1024];

int printk(const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(printk_buf, fmt, args);
	va_end(args);

	console_print(printk_buf);
	return i;
}

extern int sys_write_little(const char * buf)
{
	console_print(buf);
	return 100;
}
