#ifndef _SEGMENT_H_
#define _SEGMENT_H_

extern unsigned char get_fs_byte(const char * addr);
extern unsigned short get_fs_word(const unsigned short *addr);
extern unsigned int get_fs_long(const unsigned int *addr);
extern void put_fs_byte(char val,char *addr);
extern void put_fs_word(short val,short * addr);
extern void put_fs_long(unsigned int val,unsigned int * addr);
extern unsigned short get_fs();
extern unsigned short get_ds();
extern void set_fs(unsigned short val);

#endif
