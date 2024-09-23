#ifndef _IO_H_
#define _IO_H_

extern inline void outb(int value, int port);
extern inline unsigned char inb(int port);
extern inline void outb_p(int value, int port);
extern inline unsigned char inb_p(int port);


#endif
