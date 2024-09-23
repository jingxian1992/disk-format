#ifndef _CTYPE_H
#define _CTYPE_H

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

extern unsigned char _ctype[];
extern char _ctmp;

extern int isalnum(char c);
extern int isalpha(char c);
extern int iscntrl(char c);
extern int isdigit(char c);
extern int isgraph(char c);
extern int islower(char c);
extern int isprint(char c);
extern int ispunct(char c);
extern int isspace(char c);
extern int isupper(char c);
extern int isxdigit(char c);
extern int isascii(char c);
extern char toascii(char c);
extern char tolower(char c);
extern char toupper(char c);

#endif
