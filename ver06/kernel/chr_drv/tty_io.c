#include <stdio.h>
#include <errno.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <sys_call.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <ctype.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define ALRMMASK 0x00002000
#define QUEUES 54

extern int kill_pg(int pgrp, int sig, int priv);
extern int is_orphaned_pgrp(int pgrp);

struct tty_queue tty_queues[QUEUES];
struct tty_struct tty_table[256];

struct tty_queue * table_list[] =
{
	tty_queues + 0, tty_queues + 1,
	tty_queues + 24, tty_queues + 25,
	tty_queues + 27, tty_queues + 28
};

void change_console(unsigned int new_console)
{
	if (new_console == fg_console || new_console >= NR_CONSOLES)
		return;
	fg_console = new_console;
	table_list[0] = tty_queues + 0 + fg_console * 3;
	table_list[1] = tty_queues + 1 + fg_console * 3;
	update_screen();
}

void sleep_if_empty(struct tty_queue * queue)
{
	cli();
	while (!(current->signal & ~current->blocked) && EMPTY(queue))
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

void sleep_if_full(struct tty_queue * queue)
{
	if (!FULL(queue))
		return;
	cli();
	while (!(current->signal & ~current->blocked) && LEFT(queue)<128)
		interruptible_sleep_on(&queue->proc_list);
	sti();
}

void wait_for_keypress(void)
{
	sleep_if_empty(tty_table[fg_console].secondary);
}

void copy_to_cooked(struct tty_struct * tty)
{
	char c;
	int tty_index;
	
	tty_index = tty - tty_table;

	if (!(tty->read_q || tty->write_q || tty->secondary)) {
		panic("copy_to_cooked: missing queues\n\r");
	}
	while (1) {
		if (EMPTY(tty->read_q))
			break;
		if (FULL(tty->secondary))
			break;
		GETCH(tty->read_q, &c);
		if (c==13) {
			if (I_CRNL(tty))
				c=10;
			else if (I_NOCR(tty))
				continue;
		} else if (c==10 && I_NLCR(tty))
			c=13;
		if (I_UCLC(tty))
		{
			c=tolower(c);
		}
		if (L_CANON(tty)) {
			if ((KILL_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c==KILL_CHAR(tty))) {
				/* deal with killing the input line */
				while(!(EMPTY(tty->secondary) ||
				        (c=LAST(tty->secondary))==10 ||
				        ((EOF_CHAR(tty) != _POSIX_VDISABLE) &&
					 (c==EOF_CHAR(tty))))) {
					if (L_ECHO(tty)) {
						if (c<32)
							PUTCH(127,tty->write_q);
						PUTCH(127,tty->write_q);
						tty->write(tty);
					}
					DEC(&tty->secondary->head);
				}
				continue;
			}
			if ((ERASE_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c==ERASE_CHAR(tty))) {
				if (EMPTY(tty->secondary) ||
				   (c=LAST(tty->secondary))==10 ||
				   ((EOF_CHAR(tty) != _POSIX_VDISABLE) &&
				    (c==EOF_CHAR(tty))))
					continue;
				if (L_ECHO(tty)) {
					if (c<32)
						PUTCH(127,tty->write_q);
					PUTCH(127,tty->write_q);
					tty->write(tty);
				}
				DEC(&tty->secondary->head);
				continue;
			}
		}
		if (I_IXON(tty)) {
			if ((STOP_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c==STOP_CHAR(tty))) {
				tty->stopped=1;
				tty->write(tty);
				continue;
			}
			if ((START_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c==START_CHAR(tty))) {
				tty->stopped=0;
				tty->write(tty);
				continue;
			}
		}
		if (L_ISIG(tty))
		{
			;
		}
		if (c==10 || (EOF_CHAR(tty) != _POSIX_VDISABLE &&
			      c==EOF_CHAR(tty)))
			tty->secondary->data++;
		if (L_ECHO(tty)) {
			if (c==10) {
				PUTCH(10,tty->write_q);
				PUTCH(13,tty->write_q);
			}
			 else if (c<32) {
				if (L_ECHOCTL(tty)) {
					PUTCH('^',tty->write_q);
					PUTCH(c+64,tty->write_q);
				}
				else
					PUTCH(c,tty->write_q);
			} else
				PUTCH(c,tty->write_q);
			tty->write(tty);
		}
		
		if (!(10 == c && 0 <= tty_index && tty_index < MAX_CONSOLES)) {
			PUTCH(c,tty->secondary);
		} else {
			shell_manage();
		}
	}
	wake_up(&tty->secondary->proc_list);
}

/*
 * Called when we need to send a SIGTTIN or SIGTTOU to our process
 * group
 * 
 * We only request that a system call be restarted if there was if the 
 * default signal handler is being used.  The reason for this is that if
 * a job is catching SIGTTIN or SIGTTOU, the signal handler may not want 
 * the system call to be restarted blindly.  If there is no way to reset the
 * terminal pgrp back to the current pgrp (perhaps because the controlling
 * tty has been released on logout), we don't want to be in an infinite loop
 * while restarting the system call, and have it always generate a SIGTTIN
 * or SIGTTOU.  The default signal handler will cause the process to stop
 * thus avoiding the infinite loop problem.  Presumably the job-control
 * cognizant parent will fix things up before continuging its child process.
 */
int tty_signal(int sig, struct tty_struct *tty)
{
	if (is_orphaned_pgrp(current->pgrp))
		return -EIO;		/* don't stop an orphaned pgrp */
	(void) kill_pg(current->pgrp,sig,1);
	if ((current->blocked & (1<<(sig-1))) ||
	    ((int) current->sigaction[sig-1].sa_handler == 1)) 
		return -EIO;		/* Our signal will be ignored */
	else if (current->sigaction[sig-1].sa_handler)
		return -EINTR;		/* We _will_ be interrupted :-) */
	else
		return -ERESTARTSYS;	/* We _will_ be interrupted :-) */
					/* (but restart after we continue) */
}

int tty_read(unsigned channel, char * buf, int nr)
{
	struct tty_struct * tty;
	struct tty_struct * other_tty = NULL;
	char c, * b=buf;
	int minimum,time;

	if (channel > 255)
		return -EIO;
	tty = TTY_TABLE(channel);
	if (!(tty->write_q || tty->read_q || tty->secondary))
		return -EIO;
	if ((current->tty == channel) && (tty->pgrp != current->pgrp)) 
		return(tty_signal(SIGTTIN, tty));
	if (channel & 0x80)
		other_tty = tty_table + (channel ^ 0x40);
	time = 10L*tty->termios.c_cc[VTIME];
	minimum = tty->termios.c_cc[VMIN];
	if (L_CANON(tty)) {
		minimum = nr;
		current->timeout = 0xffffffff;
		time = 0;
	} else if (minimum)
		current->timeout = 0xffffffff;
	else {
		minimum = nr;
		if (time)
			current->timeout = time + jiffies;
		time = 0;
	}
	if (minimum>nr)
		minimum = nr;
	while (nr>0) {
		if (other_tty)
			other_tty->write(other_tty);
		cli();
		if (EMPTY(tty->secondary) || (L_CANON(tty) &&
		    !FULL(tty->read_q) && !tty->secondary->data)) {
			if (!current->timeout ||
			  (current->signal & ~current->blocked)) {
			  	sti();
				break;
			}
			if (IS_A_PTY_SLAVE(channel) && C_HUP(other_tty))
				break;
			interruptible_sleep_on(&tty->secondary->proc_list);
			sti();
			continue;
		}
		sti();
		do {
			GETCH(tty->secondary, &c);
			if ((EOF_CHAR(tty) != _POSIX_VDISABLE &&
			     c==EOF_CHAR(tty)) || c==10)
				tty->secondary->data--;
			if ((EOF_CHAR(tty) != _POSIX_VDISABLE &&
			     c==EOF_CHAR(tty)) && L_CANON(tty))
				break;
			else {
				put_fs_byte(c,b++);
				if (!--nr)
					break;
			}
			if (c==10 && L_CANON(tty))
				break;
		} while (nr>0 && !EMPTY(tty->secondary));
		wake_up(&tty->read_q->proc_list);
		if (time)
			current->timeout = time+jiffies;
		if (L_CANON(tty) || b-buf >= minimum)
			break;
	}
	current->timeout = 0;
	if ((current->signal & ~current->blocked) && !(b-buf))
		return -ERESTARTSYS;
	return (b-buf);
}

int tty_write(unsigned channel, char * buf, int nr)
{
	static cr_flag=0;
	struct tty_struct * tty;
	char c, *b=buf;

	if (channel > 255)
		return -EIO;
	tty = TTY_TABLE(channel);
	if (!(tty->write_q || tty->read_q || tty->secondary))
		return -EIO;
	if (L_TOSTOP(tty) && 
	    (current->tty == channel) && (tty->pgrp != current->pgrp)) 
		return(tty_signal(SIGTTOU, tty));
	while (nr>0) {
		sleep_if_full(tty->write_q);
		if (current->signal & ~current->blocked)
			break;
		while (nr>0 && !FULL(tty->write_q)) {
			c=get_fs_byte(b);
			if (O_POST(tty)) {
				if (c=='\r' && O_CRNL(tty))
					c='\n';
				else if (c=='\n' && O_NLRET(tty))
					c='\r';
				if (c=='\n' && !cr_flag && O_NLCR(tty)) {
					cr_flag = 1;
					PUTCH(13,tty->write_q);
					continue;
				}
				if (O_LCUC(tty))
					c=toupper(c);
			}
			b++; nr--;
			cr_flag = 0;
			PUTCH(c,tty->write_q);
		}
		tty->write(tty);
		if (nr>0)
			schedule();
	}
	return (b-buf);
}

do_tty_interrupt(int tty)
{
	struct tty_struct * tmp;
	tmp = TTY_TABLE(tty);
	copy_to_cooked(tmp);
}

void chr_dev_init(void)
{
}

void tty_init(void)
{
	int i, j;
	
	for (i=0; i<QUEUES; i++)
	{
		tty_queues[i].data = 0;
		tty_queues[i].head = 0;
		tty_queues[i].tail = 0;
		tty_queues[i].proc_list = NULL;
		for (j=0; j<TTY_BUF_SIZE; j++)
		{
			tty_queues[i].buf[j] = 0;
		}
	}
	
	tty_queues[24].data = 0x3F8;
	tty_queues[25].data = 0x3F8;
	tty_queues[27].data = 0x2F8;
	tty_queues[28].data = 0x2F8;
	
	for (i=0; i<256; i++)
	{
		tty_table[i].termios.c_iflag = 0;
		tty_table[i].termios.c_oflag = 0;
		tty_table[i].termios.c_cflag = 0;
		tty_table[i].termios.c_lflag = 0;
		tty_table[i].termios.c_line = 0;
		
		tty_table[i].termios.c_cc[0] = 003;
		tty_table[i].termios.c_cc[1] = 034;
		tty_table[i].termios.c_cc[2] = 0177;
		tty_table[i].termios.c_cc[3] = 025;
		tty_table[i].termios.c_cc[4] = 004;
		tty_table[i].termios.c_cc[5] = 0;
		tty_table[i].termios.c_cc[6] = 1;
		tty_table[i].termios.c_cc[7] = 0;
		tty_table[i].termios.c_cc[8] = 021;
		tty_table[i].termios.c_cc[9] = 023;
		tty_table[i].termios.c_cc[10] = 032;
		tty_table[i].termios.c_cc[11] = 0;
		tty_table[i].termios.c_cc[12] = 022;
		tty_table[i].termios.c_cc[13] = 017;
		tty_table[i].termios.c_cc[14] = 027;
		tty_table[i].termios.c_cc[15] = 026;
		tty_table[i].termios.c_cc[16] = 0;
		tty_table[i].pgrp = 0;
		tty_table[i].session = 0;
		tty_table[i].stopped = 0;
		tty_table[i].write = NULL;
		tty_table[i].read_q = NULL;
		tty_table[i].write_q = NULL;
		tty_table[i].secondary = NULL;		
	}
	
	con_init();
	keyboard_init();
	
	for (i=0; i<NR_CONSOLES; i++)
	{
		tty_table[i].termios.c_iflag = ICRNL;
		tty_table[i].termios.c_oflag = OPOST | ONLCR;
		tty_table[i].termios.c_lflag = IXON | ISIG | ICANON | ECHO | ECHOCTL | ECHOKE;

		tty_table[i].write = con_write;
		tty_table[i].read_q = tty_queues + 0 + i*3;
		tty_table[i].write_q = tty_queues + 1 + i*3;
		tty_table[i].secondary = tty_queues + 2 + i*3;
	}
	
	for (i = 0 ; i<NR_SERIALS ; i++)
	{
		tty_table[64 + i].termios.c_cflag = B2400 | CS8;
		tty_table[64 + i].write = rs_write;
		tty_table[64 + i].read_q = tty_queues + 24 + i * 3;
		tty_table[64 + i].write_q = tty_queues + 25 + i * 3;
		tty_table[64 + i].secondary = tty_queues + 26 + i * 3;
	}
	
	for (i = 0 ; i<NR_PTYS ; i++)
	{
		tty_table[128 + i].termios.c_cflag = B9600 | CS8;
		tty_table[128 + i].write = mpty_write;
		tty_table[128 + i].read_q = tty_queues + 30 + i * 3;
		tty_table[128 + i].write_q = tty_queues + 31 + i * 3;
		tty_table[128 + i].secondary = tty_queues + 32 + i * 3;
		
		tty_table[192 + i].termios.c_cflag = B9600 | CS8;
		tty_table[192 + i].termios.c_lflag = IXON | ISIG | ICANON;
		tty_table[192 + i].write = spty_write;
		tty_table[192 + i].read_q = tty_queues + 42 + i * 3;
		tty_table[192 + i].write_q = tty_queues + 43 + i * 3;
		tty_table[192 + i].secondary = tty_queues + 44 + i * 3;
	}
	
	rs_init();
	printk("\t%d virtual consoles\n",NR_CONSOLES);
	printk("\t%d pty's\n",NR_PTYS);
}

struct tty_struct * TTY_TABLE(int nr)
{
	int num;
	struct tty_struct * tmp;
	
	if (nr < 0 || nr >= 256)
		panic("\tbad tty number.\n");
	
	if (nr)
	{
		if (nr < 64)
		{
			num = nr - 1;
		}
		else
		{
			num = nr;
		}
	}
	else
	{
		num = fg_console;
	}
	
	tmp = tty_table + num;
	return tmp;
}

extern int IS_A_CONSOLE(int min)
{
	int res;
	res = (((min) & 0xC0) == 0x00);
	return res;
}

extern int IS_A_SERIAL(int min)
{
	int res;
	res = (((min) & 0xC0) == 0x40);
	return res;
}

extern int IS_A_PTY(int min)
{
	int res, num;
	num = ((min) & 0x80);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int IS_A_PTY_MASTER(int min)
{
	int res;
	res = (((min) & 0xC0) == 0x80);
	return res;
}

extern int IS_A_PTY_SLAVE(int min)
{
	int res;
	res = (((min) & 0xC0) == 0xC0);
	return res;
}

extern int PTY_OTHER(int min)
{
	int res, num;
	num = ((min) ^ 0x40);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

void INC(int * a)
{
	(*a) = ((*a) + 1) & (TTY_BUF_SIZE - 1);
}

void DEC(int * a)
{
	(*a) = ((*a) - 1) & (TTY_BUF_SIZE - 1);
}

int EMPTY(struct tty_queue * a)
{
	if (a->head == a->tail)
		return 1;
	else
		return 0;
}

int LEFT(struct tty_queue * a)
{
	int num;
	num = (a->tail - a->head - 1) & (TTY_BUF_SIZE - 1);
	return num;
}

char LAST(struct tty_queue * a)
{
	char tmp;
	int i;
	i = (a->head - 1) & (TTY_BUF_SIZE - 1);
	tmp = a->buf[i];
	return tmp;
}

int FULL(struct tty_queue * a)
{
	int num;
	num = (a->tail - a->head - 1) & (TTY_BUF_SIZE - 1);
	return (!num);
}

int CHARS(struct tty_queue * a)
{
	int num;
	num = (a->head - a->tail) & (TTY_BUF_SIZE - 1);
	return num;
}

void GETCH(struct tty_queue * queue, char * c)
{
	(*c) = queue->buf[queue->tail];
	queue->tail = (queue->tail + 1) & (TTY_BUF_SIZE - 1);
}

void PUTCH(char c, struct tty_queue * queue)
{
	queue->buf[queue->head] = c;
	queue->head = (queue->head + 1) & (TTY_BUF_SIZE - 1);
}

unsigned char INTR_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VINTR];
	return ch;
}

unsigned char QUIT_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VQUIT];
	return ch;
}

unsigned char ERASE_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VERASE];
	return ch;
}

unsigned char KILL_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VKILL];
	return ch;
}

unsigned char EOF_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VEOF];
	return ch;
}

unsigned char START_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VSTART];
	return ch;
}

unsigned char STOP_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VSTOP];
	return ch;
}

unsigned char SUSPEND_CHAR(struct tty_struct * tty)
{
	unsigned char ch;
	ch = tty->termios.c_cc[VSUSP];
	return ch;
}

unsigned int L_CANON(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & ICANON;
	return tmp;
}

unsigned int L_ISIG(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & ISIG;
	return tmp;
}

unsigned int L_ECHO(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & ECHO;
	return tmp;
}

unsigned int L_ECHOE(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & ECHOE;
	return tmp;
}

unsigned int L_ECHOK(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & ECHOK;
	return tmp;
}

unsigned int L_ECHOCTL(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & ECHOCTL;
	return tmp;
}

unsigned int L_ECHOKE(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & ECHOKE;
	return tmp;
}

unsigned int L_TOSTOP(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_lflag & TOSTOP;
	return tmp;
}

unsigned int I_UCLC(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_iflag & IUCLC;
	return tmp;
}

unsigned int I_NLCR(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_iflag & INLCR;
	return tmp;
}

unsigned int I_CRNL(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_iflag & ICRNL;
	return tmp;
}

unsigned int I_NOCR(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_iflag & IGNCR;
	return tmp;
}

unsigned int I_IXON(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_iflag & IXON;
	return tmp;
}

unsigned int O_POST(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_oflag & OPOST;
	return tmp;
}

unsigned int O_NLCR(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_oflag & ONLCR;
	return tmp;
}

unsigned int O_CRNL(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_oflag & OCRNL;
	return tmp;
}

unsigned int O_NLRET(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_oflag & ONLRET;
	return tmp;
}

unsigned int O_LCUC(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_oflag & OLCUC;
	return tmp;
}

unsigned int C_SPEED(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_cflag & CBAUD;
	return tmp;
}

int C_HUP(struct tty_struct * tty)
{
	unsigned int tmp;
	tmp = tty->termios.c_cflag & CBAUD;
	if (B0 == tmp)
		return 1;
	else
		return 0; 
}

extern int isalnum(char c)
{
	int res, num;
	num =  (_ctype+1)[c] & (_U|_L|_D);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isalpha(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_U|_L);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int iscntrl(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_C);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isdigit(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_D);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isgraph(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_P|_U|_L|_D);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int islower(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_L);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isprint(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_P|_U|_L|_D|_SP);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int ispunct(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_P);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isspace(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_S);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isupper(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_U);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isxdigit(char c)
{
	int res, num;
	num = (_ctype+1)[c] & (_D|_X);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern int isascii(char c)
{
	int res, num;
	num = (((unsigned char) c)<=0x7f);
	if (num)
		res = 1;
	else
		res = 0;
	return res;
}

extern char toascii(char c)
{
	char res;
	unsigned int num;
	num = ((unsigned int)c) & 0x7F;
	res = (char)num;
	return res;
}

extern char tolower(char c)
{
	_ctmp = c;
	if (isupper(_ctmp))
		_ctmp = _ctmp + ('a'-'A');
	else
		;
	return _ctmp;
}

extern char toupper(char c)
{
	_ctmp=c;
	if (islower(_ctmp))
		_ctmp = _ctmp - ('a'-'A');
	else
		;
	return _ctmp;
}
 












