#ifndef _MM_H_
#define _MM_H_

#define LOW_MEM 0x300000
#define TOTAL_MEMORY 0x4000000
#define PAGING_MEMORY (TOTAL_MEMORY - LOW_MEM)
#define PAGING_PAGES (PAGING_MEMORY >> 12)
#define USED 100

#define PAGE_SIZE 4096

#define PAGE_DIRTY	0x40
#define PAGE_ACCESSED	0x20
#define PAGE_USER	0x04
#define PAGE_RW		0x02
#define PAGE_PRESENT	0x01

#define read_swap_page(nr,buffer) ll_rw_page(READ,SWAP_DEV,(nr),(buffer));
#define write_swap_page(nr,buffer) ll_rw_page(WRITE,SWAP_DEV,(nr),(buffer));

extern unsigned int HIGH_MEMORY;
extern unsigned char mem_map [ PAGING_PAGES ];

extern unsigned int get_free_page(void);
extern unsigned int put_dirty_page(unsigned int page,unsigned int address);
extern void free_page(unsigned int addr);
extern void invalidate(void);
extern volatile void oom(void);
extern void invalidate(void);
extern void copy_page(unsigned int from, unsigned int to);
extern unsigned int MAP_NR(unsigned int addr);

#endif








