#include <asm/segment.h>

extern unsigned char get_fs_byte(const char * addr)
{
	unsigned register char _v;

	__asm__ __volatile__("movb %%fs:%1,%0":"=r" (_v):"m" (*addr));
	return _v;
}

extern unsigned short get_fs_word(const unsigned short *addr)
{
	unsigned short _v;

	__asm__ __volatile__("movw %%fs:%1,%0":"=r" (_v):"m" (*addr));
	return _v;
}

extern unsigned int get_fs_long(const unsigned int *addr)
{
	unsigned int _v;

	__asm__ __volatile__("movl %%fs:%1,%0":"=r" (_v):"m" (*addr));
	return _v;
}

extern void put_fs_byte(char val,char *addr)
{
__asm__ __volatile__("movb %0,%%fs:%1"::"r" (val),"m" (*addr));
}

extern void put_fs_word(short val,short * addr)
{
__asm__ __volatile__("movw %0,%%fs:%1"::"r" (val),"m" (*addr));
}

extern void put_fs_long(unsigned int val,unsigned int * addr)
{
__asm__ __volatile__("movl %0,%%fs:%1"::"r" (val),"m" (*addr));
}

/*
 * Someone who knows GNU asm better than I should double check the followig.
 * It seems to work, but I don't know if I'm doing something subtly wrong.
 * --- TYT, 11/24/91
 * [ nothing wrong here, Linus ]
 */

extern unsigned short get_fs() 
{
	unsigned short _v;
	_v = 0;
	__asm__ __volatile__("mov %%fs,%%ax":"=a" (_v):);

	return _v;
}

extern unsigned short get_ds() 
{
	unsigned short _v;
	__asm__ __volatile__("mov %%ds,%%ax":"=a" (_v):);
	return _v;
}

extern void set_fs(unsigned short val)
{
	__asm__ __volatile__("mov %0,%%fs"::"a" ((unsigned short) val));
}

