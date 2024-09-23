#include <asm/io.h>

extern inline void outb(int value, int port)
{
__asm__ __volatile__("outb %%al,%%dx"::"a" (value),"d" (port));
}

extern inline unsigned char inb(int port)
{
	unsigned char _v;
__asm__ __volatile__ ("inb %%dx,%%al":"=a" (_v):"d" (port));
	return _v;
}

extern inline void outb_p(int value, int port)
{
__asm__ __volatile__("outb %%al,%%dx\n\t"
		"\tjmp 1f\n\t"
		"1:\tjmp 1f\n\t"
		"1:"::"a" (value),"d" (port));
}

extern inline unsigned char inb_p(int port)
{
	unsigned char _v;
__asm__ __volatile__ ("inb %%dx,%%al\n\t"
	"\tjmp 1f\n\t"
	"1:\tjmp 1f\n\t"
	"1:":"=a" (_v):"d" (port));
	return _v;
}






