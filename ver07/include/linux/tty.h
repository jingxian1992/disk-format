#ifndef _TTY_H
#define _TTY_H

#ifndef MAX_CONSOLES
#define MAX_CONSOLES	8
#endif

#ifndef  NR_SERIALS
#define NR_SERIALS	2
#endif

#ifndef NR_PTYS
#define NR_PTYS		4
#endif

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0
#endif

extern int NR_CONSOLES;

#define TTY_BUF_SIZE 1024

#define TCGETS		0x5401
#define TCSETS		0x5402
#define TCSETSW		0x5403
#define TCSETSF		0x5404
#define TCGETA		0x5405
#define TCSETA		0x5406
#define TCSETAW		0x5407
#define TCSETAF		0x5408
#define TCSBRK		0x5409
#define TCXONC		0x540A
#define TCFLSH		0x540B
#define TIOCEXCL	0x540C
#define TIOCNXCL	0x540D
#define TIOCSCTTY	0x540E
#define TIOCGPGRP	0x540F
#define TIOCSPGRP	0x5410
#define TIOCOUTQ	0x5411
#define TIOCSTI		0x5412
#define TIOCGWINSZ	0x5413
#define TIOCSWINSZ	0x5414
#define TIOCMGET	0x5415
#define TIOCMBIS	0x5416
#define TIOCMBIC	0x5417
#define TIOCMSET	0x5418
#define TIOCGSOFTCAR	0x5419
#define TIOCSSOFTCAR	0x541A
#define FIONREAD	0x541B
#define TIOCINQ		FIONREAD

#define NCCS 17
struct termios {
	unsigned int c_iflag;		/* input mode flags */
	unsigned int c_oflag;		/* output mode flags */
	unsigned int c_cflag;		/* control mode flags */
	unsigned int c_lflag;		/* local mode flags */
	unsigned char c_line;			/* line discipline */
	unsigned char c_cc[NCCS];		/* control characters */
};

/* c_cc characters */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

/* c_iflag bits */
#define IGNBRK	0000001
#define BRKINT	0000002
#define IGNPAR	0000004
#define PARMRK	0000010
#define INPCK	0000020
#define ISTRIP	0000040
#define INLCR	0000100
#define IGNCR	0000200
#define ICRNL	0000400
#define IUCLC	0001000
#define IXON	0002000
#define IXANY	0004000
#define IXOFF	0010000
#define IMAXBEL	0020000

/* c_oflag bits */
#define OPOST	0000001
#define OLCUC	0000002
#define ONLCR	0000004
#define OCRNL	0000010
#define ONOCR	0000020
#define ONLRET	0000040
#define OFILL	0000100
#define OFDEL	0000200
#define NLDLY	0000400
#define   NL0	0000000
#define   NL1	0000400
#define CRDLY	0003000
#define   CR0	0000000
#define   CR1	0001000
#define   CR2	0002000
#define   CR3	0003000
#define TABDLY	0014000
#define   TAB0	0000000
#define   TAB1	0004000
#define   TAB2	0010000
#define   TAB3	0014000
#define   XTABS	0014000
#define BSDLY	0020000
#define   BS0	0000000
#define   BS1	0020000
#define VTDLY	0040000
#define   VT0	0000000
#define   VT1	0040000
#define FFDLY	0040000
#define   FF0	0000000
#define   FF1	0040000

/* c_cflag bit meaning */
#define CBAUD	0000017
#define  B0	0000000		/* hang up */
#define  B50	0000001
#define  B75	0000002
#define  B110	0000003
#define  B134	0000004
#define  B150	0000005
#define  B200	0000006
#define  B300	0000007
#define  B600	0000010
#define  B1200	0000011
#define  B1800	0000012
#define  B2400	0000013
#define  B4800	0000014
#define  B9600	0000015
#define  B19200	0000016
#define  B38400	0000017
#define EXTA B19200
#define EXTB B38400
#define CSIZE	0000060
#define   CS5	0000000
#define   CS6	0000020
#define   CS7	0000040
#define   CS8	0000060
#define CSTOPB	0000100
#define CREAD	0000200
#define PARENB	0000400
#define PARODD	0001000
#define HUPCL	0002000
#define CLOCAL	0004000
#define CIBAUD	03600000		/* input baud rate (not used) */
#define CRTSCTS	020000000000		/* flow control */

/* c_lflag bits */
#define ISIG	0000001
#define ICANON	0000002
#define XCASE	0000004
#define ECHO	0000010
#define ECHOE	0000020
#define ECHOK	0000040
#define ECHONL	0000100
#define NOFLSH	0000200
#define TOSTOP	0000400
#define ECHOCTL	0001000
#define ECHOPRT	0002000
#define ECHOKE	0004000
#define FLUSHO	0010000
#define PENDIN	0040000
#define IEXTEN	0100000

/* modem lines */
#define TIOCM_LE	0x001
#define TIOCM_DTR	0x002
#define TIOCM_RTS	0x004
#define TIOCM_ST	0x008
#define TIOCM_SR	0x010
#define TIOCM_CTS	0x020
#define TIOCM_CAR	0x040
#define TIOCM_RNG	0x080
#define TIOCM_DSR	0x100
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RI	TIOCM_RNG

/* tcflow() and TCXONC use these */
#define	TCOOFF		0
#define	TCOON		1
#define	TCIOFF		2
#define	TCION		3

/* tcflush() and TCFLSH use these */
#define	TCIFLUSH	0
#define	TCOFLUSH	1
#define	TCIOFLUSH	2

/* tcsetattr uses these */
#define	TCSANOW		0
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

struct tty_queue
{
	unsigned int data;
	unsigned int head;
	unsigned int tail;
	struct task_struct * proc_list;
	char buf[TTY_BUF_SIZE];
};

struct tty_struct
{
	struct termios termios;
	int pgrp;
	int session;
	int stopped;
	void (*write)(struct tty_struct * tty);
	struct tty_queue * read_q;
	struct tty_queue * write_q;
	struct tty_queue * secondary;
};

extern struct tty_queue tty_queues[];
extern struct tty_struct tty_table[];
extern int fg_console;

struct shell_struct
{
	int cur;
	char cmd_buf[1024];
};
struct shell_struct shell[MAX_CONSOLES];
#define SHELL_INFO "[root@dumpling]:#  "

#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

extern void rs_init(void);
extern inline void gotoxy(int currcons, unsigned int new_x, unsigned new_y);
extern inline void set_cursor(int currcons);
extern void con_init(void);
extern void csi_J(int currcons, int vpar);
extern void csi_K(int currcons, int vpar);
void update_screen(void);
extern void tty_init(void);
extern int tty_read(unsigned c, char * buf, int n);
extern int tty_write(unsigned c, char * buf, int n);
extern void con_write(struct tty_struct * tty);
extern void rs_write(struct tty_struct * tty);
extern void mpty_write(struct tty_struct * tty);
extern void spty_write(struct tty_struct * tty);
extern void copy_to_cooked(struct tty_struct * tty);
extern void update_screen(void);

extern void keyboard_interrupt(void);
extern struct tty_struct * TTY_TABLE(int nr);
extern int IS_A_CONSOLE(int min);
extern int IS_A_SERIAL(int min);
extern int IS_A_PTY(int min);
extern int IS_A_PTY_MASTER(int min);
extern int IS_A_PTY_SLAVE(int min);
extern int PTY_OTHER(int min);
extern void INC(int * a);
extern void DEC(int * a);
extern int EMPTY(struct tty_queue * a);
extern int LEFT(struct tty_queue * a);
extern char LAST(struct tty_queue * a);
extern int FULL(struct tty_queue * a);
extern int CHARS(struct tty_queue * a);
extern void GETCH(struct tty_queue * queue, char * c);
extern void PUTCH(char c, struct tty_queue * queue);

extern unsigned char INTR_CHAR(struct tty_struct * tty);
extern unsigned char QUIT_CHAR(struct tty_struct * tty);
extern unsigned char ERASE_CHAR(struct tty_struct * tty);
extern unsigned char KILL_CHAR(struct tty_struct * tty);
extern unsigned char EOF_CHAR(struct tty_struct * tty);
extern unsigned char START_CHAR(struct tty_struct * tty);
extern unsigned char STOP_CHAR(struct tty_struct * tty);
extern unsigned char SUSPEND_CHAR(struct tty_struct * tty);

extern unsigned int L_CANON(struct tty_struct * tty);
extern unsigned int L_ISIG(struct tty_struct * tty);
extern unsigned int L_ECHO(struct tty_struct * tty);
extern unsigned int L_ECHOE(struct tty_struct * tty);
extern unsigned int L_ECHOK(struct tty_struct * tty);
extern unsigned int L_ECHOCTL(struct tty_struct * tty);
extern unsigned int L_ECHOKE(struct tty_struct * tty);
extern unsigned int L_TOSTOP(struct tty_struct * tty);

extern unsigned int I_UCLC(struct tty_struct * tty);
extern unsigned int I_NLCR(struct tty_struct * tty);
extern unsigned int I_CRNL(struct tty_struct * tty);
extern unsigned int I_NOCR(struct tty_struct * tty);
extern unsigned int I_IXON(struct tty_struct * tty);

extern unsigned int O_POST(struct tty_struct * tty);
extern unsigned int O_NLCR(struct tty_struct * tty);
extern unsigned int O_CRNL(struct tty_struct * tty);
extern unsigned int O_NLRET(struct tty_struct * tty);
extern unsigned int O_LCUC(struct tty_struct * tty);

extern unsigned int C_SPEED(struct tty_struct * tty);
extern int C_HUP(struct tty_struct * tty);

extern int sys_start_shell(void);
extern void shell_manage(void);
extern void cmd_clear(void);

#endif

















