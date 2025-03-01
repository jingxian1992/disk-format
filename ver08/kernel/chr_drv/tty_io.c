/*
 *  linux/kernel/tty_io.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * 'tty_io.c' gives an orthogonal feeling to tty's, be they consoles
 * or rs-channels. It also implements echoing, cooked mode etc.
 *
 * Kill-line thanks to John T Kohl, who also corrected VMIN = VTIME = 0.
 */

#define ALRMMASK (1<<(SIGALRM-1))

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/segment.h>
#include <asm/system.h>

extern int kill_pg(int pgrp, int sig, int priv);
extern int is_orphaned_pgrp(int pgrp);
extern void keyboard_interrupt(void);

#define _L_FLAG(tty,f)	((tty)->termios.c_lflag & f)
#define _I_FLAG(tty,f)	((tty)->termios.c_iflag & f)
#define _O_FLAG(tty,f)	((tty)->termios.c_oflag & f)

#define L_CANON(tty)	_L_FLAG((tty),ICANON)
#define L_ISIG(tty)	_L_FLAG((tty),ISIG)
#define L_ECHO(tty)	_L_FLAG((tty),ECHO)
#define L_ECHOE(tty)	_L_FLAG((tty),ECHOE)
#define L_ECHOK(tty)	_L_FLAG((tty),ECHOK)
#define L_ECHOCTL(tty)	_L_FLAG((tty),ECHOCTL)
#define L_ECHOKE(tty)	_L_FLAG((tty),ECHOKE)
#define L_TOSTOP(tty)	_L_FLAG((tty),TOSTOP)

#define I_UCLC(tty)	_I_FLAG((tty),IUCLC)
#define I_NLCR(tty)	_I_FLAG((tty),INLCR)
#define I_CRNL(tty)	_I_FLAG((tty),ICRNL)
#define I_NOCR(tty)	_I_FLAG((tty),IGNCR)
#define I_IXON(tty)	_I_FLAG((tty),IXON)

#define O_POST(tty)	_O_FLAG((tty),OPOST)
#define O_NLCR(tty)	_O_FLAG((tty),ONLCR)
#define O_CRNL(tty)	_O_FLAG((tty),OCRNL)
#define O_NLRET(tty)	_O_FLAG((tty),ONLRET)
#define O_LCUC(tty)	_O_FLAG((tty),OLCUC)

#define C_SPEED(tty)	((tty)->termios.c_cflag & CBAUD)
#define C_HUP(tty)	(C_SPEED((tty)) == B0)

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


struct tty_queue tty_queues[QUEUES];
struct tty_struct tty_table[128];

#define con_queues tty_queues
#define rs_queues ((3*MAX_CONSOLES) + tty_queues)
#define con_table tty_table
#define rs_table (64+tty_table)

/*
 * these are the tables used by the machine code handlers.
 * you can implement virtual consoles.
 */
struct tty_queue * table_list[]={
	con_queues + 0, con_queues + 1,
	rs_queues + 0, rs_queues + 1,
	rs_queues + 3, rs_queues + 4
	};

void change_console(unsigned int new_console)
{
	if (new_console == fg_console || new_console >= NR_CONSOLES)
		return;
	fg_console = new_console;
	table_list[0] = con_queues + 0 + fg_console*3;
	table_list[1] = con_queues + 1 + fg_console*3;
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
		printk("copy_to_cooked: missing queues\n\r");
		return;
	}
	while (1) {
		if (EMPTY(tty->read_q))
			break;
		if (FULL(tty->secondary))
			break;
		GETCH(tty->read_q,c);
		if (c==13) {
			if (I_CRNL(tty))
				c=10;
			else if (I_NOCR(tty))
				continue;
		} else if (c==10 && I_NLCR(tty))
			c=13;
		if (I_UCLC(tty))
			c=tolower(c);
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
					DEC(tty->secondary->head);
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
				DEC(tty->secondary->head);
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
		if (L_ISIG(tty)) {
			if ((INTR_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c==INTR_CHAR(tty))) {
				kill_pg(tty->pgrp, SIGINT, 1);
				continue;
			}
			if ((QUIT_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c==QUIT_CHAR(tty))) {
				kill_pg(tty->pgrp, SIGQUIT, 1);
				continue;
			}
			if ((SUSPEND_CHAR(tty) != _POSIX_VDISABLE) &&
			    (c==SUSPEND_CHAR(tty))) {
				if (!is_orphaned_pgrp(tty->pgrp))
					kill_pg(tty->pgrp, SIGTSTP, 1);
				continue;
			}
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
			} else
				PUTCH(c,tty->write_q);
			tty->write(tty);
		}
		
		if (10 == c && 0 <= tty_index && tty_index < MAX_CONSOLES)
		{
			shell_manage();
		}
		else
		{
			PUTCH(c,tty->secondary);
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
			interruptible_sleep_on(&tty->secondary->proc_list);
			sti();
			continue;
		}
		sti();
		do {
			GETCH(tty->secondary,c);
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

/*
 * Jeh, sometimes I really like the 386.
 * This routine is called from an interrupt,
 * and there should be absolutely no problem
 * with sleeping even in an interrupt (I hope).
 * Of course, if somebody proves me wrong, I'll
 * hate intel for all time :-). We'll have to
 * be careful and see to reinstating the interrupt
 * chips before calling this, though.
 *
 * I don't think we sleep here under normal circumstances
 * anyway, which is good, as the task sleeping might be
 * totally innocent.
 */
void do_tty_interrupt(int tty)
{
	copy_to_cooked(TTY_TABLE(tty));
}

void chr_dev_init(void)
{
	unsigned char a;
	
	set_trap_gate(0x21,&keyboard_interrupt);
	outb_p(inb_p(0x21)&0xfd,0x21);
	a=inb_p(0x61);
	outb_p(a|0x80,0x61);
	outb_p(a,0x61);
}

void tty_init(void)
{
	int i, j;

	memcpy(INIT_C_CC, ORIG_INIT_C_CC, NCCS);

	for (i=0 ; i < QUEUES ; i++)
	{
		tty_queues[i].data = 0;
		tty_queues[i].head = 0;
		tty_queues[i].tail = 0;
		tty_queues[i].proc_list = NULL;
		memset(tty_queues[i].buf, 0, TTY_BUF_SIZE);
	}

	rs_queues[0].data = 0x3F8;
	rs_queues[1].data = 0x3F8;
	rs_queues[3].data = 0x2F8;
	rs_queues[4].data = 0x2F8;

	for (i=0 ; i<128 ; i++)
	{
		tty_table[i].termios.c_iflag = 0;
		tty_table[i].termios.c_oflag = 0;
		tty_table[i].termios.c_cflag = 0;
		tty_table[i].termios.c_lflag = 0;
		tty_table[i].termios.c_line = 0;
		memcpy((void *)(tty_table[i].termios.c_cc), (void *)INIT_C_CC, NCCS);
		tty_table[i].pgrp = 0;
		tty_table[i].session = 0;
		tty_table[i].stopped = 0;
		tty_table[i].write = NULL;
		tty_table[i].read_q = NULL;
		tty_table[i].write_q = NULL;
		tty_table[i].secondary = NULL;
	}
	
	for (i = 0 ; i<NR_CONSOLES ; i++)
	{
		con_table[i].termios.c_iflag = ICRNL;
		con_table[i].termios.c_oflag = OPOST|ONLCR;
		con_table[i].termios.c_lflag = IXON | ISIG | ICANON | ECHO | ECHOCTL | ECHOKE;
		tty_table[i].write = con_write;
		tty_table[i].read_q = con_queues+0+i*3;
		tty_table[i].write_q = con_queues+1+i*3;
		tty_table[i].secondary = con_queues+2+i*3;
	}

	for (i = 0 ; i<NR_SERIALS ; i++)
	{
		rs_table[i].termios.c_cflag = B2400 | CS8;
		rs_table[i].write = rs_write;
		rs_table[i].read_q = rs_queues+0+i*3;
		rs_table[i].write_q = rs_queues+1+i*3;
		rs_table[i].secondary = rs_queues+2+i*3;
	}
	rs_init();
	
	printk("\t%d virtual consoles.\n",NR_CONSOLES);
	printk("\t%d serial ttys\n",NR_SERIALS);
}





