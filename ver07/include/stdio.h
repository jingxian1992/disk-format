#ifndef _STDIO_H_
#define _STDIO_H_

typedef char *va_list;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Amount of space required in an argument list for an arg of type TYPE.
   TYPE may alternatively be an expression whose type is used.  */

#define __va_rounded_size(TYPE)  \
  (((sizeof (TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_start(AP, LASTARG) 						\
 (AP = ((char *) &(LASTARG) + __va_rounded_size (LASTARG)))

#define va_end(AP) (AP = NULL)

#define va_arg(AP, TYPE)						\
 (AP += __va_rounded_size (TYPE),					\
  *((TYPE *) (AP - __va_rounded_size (TYPE))))


extern int vsprintf(char *buf, const char *fmt, va_list args);
extern int printk(const char * fmt, ...);
extern void console_print(const char * b);
extern volatile void panic_exp(const char * s, const char * file_name, int line_num, const char * func_name);
extern void verify_area(void * addr,int count);
extern volatile void do_exit(int code);

#define panic(var) panic_exp(var, __FILE__, __LINE__, __func__)

#endif
