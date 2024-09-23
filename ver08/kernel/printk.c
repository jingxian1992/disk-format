#include <stdio.h>

static char buf[1024];

extern int printk(const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	console_print(buf);
	return i;
}
