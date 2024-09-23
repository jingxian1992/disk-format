#ifndef _HEAD_H_
#define _HEAD_H_

struct desc_struct
{
	unsigned int low4bit;
	unsigned int high4bit;
};

extern unsigned int * pg_dir;
extern struct desc_struct * idt;
extern struct desc_struct * gdt;

#endif
