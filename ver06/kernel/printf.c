#include <stdio.h>
#include <sys_call.h>

char printf_buf[1024];
extern int printf(const char * fmt, ...)
{
	va_list args;
	int i, __res;
	
	va_start(args, fmt);
	i = vsprintf(printf_buf, fmt, args);
	va_end(args);
	
	write_simple(printf_buf, i);
	return i;
}






