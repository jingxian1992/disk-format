/*
 *  NOTE!!! memcpy(dest,src,n) assumes ds=es=normal data segment. This
 *  goes for all kernel functions (ds=es=kernel space, fs=local data,
 *  gs=null), as well as for all well-behaving user programs (ds=es=
 *  user data space). This is NOT a bug, as any user program that changes
 *  es deserves to die if it isn't careful.
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

#define PAGE_DIRTY	0x40
#define PAGE_ACCESSED	0x20
#define PAGE_USER	0x04
#define PAGE_RW		0x02
#define PAGE_PRESENT	0x01

extern int buffer_memory_end, memory_end, buffer_memory_end_M;
extern int main_memory_start, memory_end_M, rd_length;
extern char * rd_start;
extern int HIGH_MEMORY;
extern unsigned char mem_map [];

extern int MAP_NR(int addr);
extern unsigned int get_free_page(void);
extern void free_page(unsigned int addr);
extern int free_page_tables(unsigned int from,unsigned int size);
extern int copy_page_tables(unsigned int from,unsigned int to,int size);
extern unsigned int put_page(unsigned int page,unsigned int address);
extern unsigned int put_dirty_page(unsigned int page, unsigned int address);
extern void un_wp_page(unsigned int * table_entry);
extern void do_wp_page(unsigned int error_code,unsigned int address);
extern void write_verify(unsigned int address);
extern void get_empty_page(unsigned int address);
extern int try_to_share(unsigned int address, struct task_struct * p);
extern int share_page(unsigned int address);
extern void do_no_page(unsigned int error_code,unsigned int address);
extern void show_mem(void);

#endif
