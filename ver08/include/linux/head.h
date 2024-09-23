#ifndef _HEAD_H_
#define _HEAD_H_

struct desc_struct
{
	unsigned int a, b;
};

unsigned int * pg_dir;
struct desc_struct * idt;
struct desc_struct * gdt;

#endif
