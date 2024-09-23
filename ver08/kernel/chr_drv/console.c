#include <stdio.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/tty.h>

int NR_CONSOLES;
int fg_console;

unsigned int	video_num_columns;
unsigned int	video_mem_base;
unsigned int	video_mem_term;
unsigned int	video_size_row;
unsigned int	video_num_lines;
unsigned short	video_port_reg;
unsigned short	video_port_val;

struct console_struct {
	unsigned short	vc_video_erase_char;	
	unsigned char	vc_attr;
	unsigned int	vc_origin;		/* Used for EGA/VGA fast scroll	*/
	unsigned int	vc_scr_end;		/* Used for EGA/VGA fast scroll	*/
	unsigned int	vc_pos;
	unsigned int	vc_x,vc_y;
	unsigned int	vc_top,vc_bottom;
	unsigned int	vc_video_mem_start;	/* Start of video RAM		*/
	unsigned int	vc_video_mem_end;	/* End of video RAM (sort of)	*/
	char *		vc_translate;
} ;
struct console_struct vc_cons [MAX_CONSOLES];

#define origin		(vc_cons[currcons].vc_origin)
#define scr_end		(vc_cons[currcons].vc_scr_end)
#define pos		(vc_cons[currcons].vc_pos)
#define top		(vc_cons[currcons].vc_top)
#define bottom		(vc_cons[currcons].vc_bottom)
#define x		(vc_cons[currcons].vc_x)
#define y		(vc_cons[currcons].vc_y)
#define attr		(vc_cons[currcons].vc_attr)
#define translate	(vc_cons[currcons].vc_translate)
#define video_mem_start	(vc_cons[currcons].vc_video_mem_start)
#define video_mem_end	(vc_cons[currcons].vc_video_mem_end)
#define video_erase_char  (vc_cons[currcons].vc_video_erase_char)

char * translations;

static unsigned short get_init_pos()
{
	unsigned short tmp;
	__asm__ __volatile__("movw $0x3d4, %%dx\n\t"
		"movb $14, %%al\n\t"
		"outb %%al, %%dx\n\t"
		"movw $0x3d5, %%dx\n\t"
		"inb %%dx, %%al\n\t"
		"movb %%al, %%bh\n\t"
	
		"movw $0x3d4, %%dx\n\t"
		"movb $15, %%al\n\t"
		"outb %%al, %%dx\n\t"
		"movw $0x3d5, %%dx\n\t"
		"inb %%dx, %%al\n\t"
		"movb %%al, %%bl"
		:"=b"(tmp)
		:"a"(0), ""(0), "d"(0)
		:);
	return tmp;
}

extern void gotoxy(int currcons, unsigned int new_x, unsigned new_y)
{
	if (new_x > video_num_columns || new_y >= video_num_lines)
		return;
	x = new_x;
	y = new_y;
	pos = origin + y * video_size_row + (x << 1);
}

static void set_origin(int currcons)
{
	unsigned int tmp_intr_flag;
	unsigned short origin_pos;
	unsigned char high, low;
	if (currcons != fg_console)
		return;

	origin_pos = (origin - video_mem_base) >> 1;
	low = (unsigned char)(0xFF & origin_pos);
	high = (unsigned char)(0xFF & (origin_pos >> 8));

	tmp_intr_flag = save_intr_flag();
	cli();
	outb_p(12, video_port_reg);
	outb_p(high, video_port_val);
	outb_p(13, video_port_reg);
	outb_p(low, video_port_val);
	restore_intr_flag(tmp_intr_flag);	
}

extern void scrup(int currcons)
{
	origin += video_size_row;
	pos += video_size_row;
	scr_end += video_size_row;
	if (scr_end > video_mem_end) {
		__asm__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl video_num_columns,%1\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" ((video_num_lines-1)*video_num_columns>>1),
			"D" (video_mem_start),
			"S" (origin)
			);
		scr_end -= origin-video_mem_start;
		pos -= origin-video_mem_start;
		origin = video_mem_start;
	} else {
		__asm__("cld\n\t"
			"rep\n\t"
			"stosw"
			::"a" (video_erase_char),
			"c" (video_num_columns),
			"D" (scr_end-video_size_row)
			);
	}
	set_origin(currcons);
}

static void lf(int currcons)
{
	if (y + 1 < bottom)
	{
		y++;
		pos += video_size_row;
		return;
	}
	scrup(currcons);
}

static void cr(int currcons)
{
	pos -= x << 1;
	x = 0;
}

static void ht(int currcons)
{
	int tmp;
	tmp = 4 - (x & 3);
	x += tmp;
	pos += tmp << 1;
	if (x >= video_num_columns)
	{
		x -= video_num_columns;
		pos -= video_size_row;
		lf(currcons);
	}
}

static void del(int currcons)
{
	if (x)
	{
		pos -= 2;
		x--;
		*(unsigned short *)pos = video_erase_char;
	}
}

extern inline void set_cursor(int currcons)
{
	unsigned int tmp_intr_flag;
	unsigned short cursor_pos;
	unsigned char high, low;
	if (currcons != fg_console)
		return;

	cursor_pos = (pos - video_mem_base) >> 1;
	low = (unsigned char)(0xFF & cursor_pos);
	high = (unsigned char)(0xFF & (cursor_pos >> 8));

	tmp_intr_flag = save_intr_flag();
	cli();
	outb_p(14, video_port_reg);
	outb_p(high, video_port_val);
	outb_p(15, video_port_reg);
	outb_p(low, video_port_val);
	restore_intr_flag(tmp_intr_flag);	
}

extern void csi_J(int currcons, int vpar)
{
	int count __asm__("cx");
	int start __asm__("di");

	switch (vpar) {
		case 0:	/* erase from cursor to end of display */
			count = (scr_end-pos)>>1;
			start = pos;
			break;
		case 1:	/* erase from start to cursor */
			count = (pos-origin)>>1;
			start = origin;
			break;
		case 2: /* erase whole display */
			count = video_num_columns * video_num_lines;
			start = origin;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
		);
}

extern void csi_K(int currcons, int vpar)
{
	int count __asm__("cx");
	int start __asm__("di");

	switch (vpar) {
		case 0:	/* erase from cursor to end of line */
			if (x>=video_num_columns)
				return;
			count = video_num_columns-x;
			start = pos;
			break;
		case 1:	/* erase from start of line to cursor */
			start = pos - (x<<1);
			count = (x<video_num_columns)?x:video_num_columns;
			break;
		case 2: /* erase whole line */
			start = pos - (x<<1);
			count = video_num_columns;
			break;
		default:
			return;
	}
	__asm__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		::"c" (count),
		"D" (start),"a" (video_erase_char)
	);
}

void con_write(struct tty_struct * tty)
{
	int nr;
	char c;
	int currcons;
     
	currcons = tty - tty_table;
	
	if ((currcons>=MAX_CONSOLES) || (currcons<0))
		panic("con_write: illegal tty");
 	   
	nr = CHARS(tty->write_q);
	
	while (nr--) {
		if (tty->stopped)
			break;
		GETCH(tty->write_q,c);
		if (c == 24 || c == 26)
			;

		if (c>31 && c<127) {
			if (x>=video_num_columns) {
				x -= video_num_columns;
				pos -= video_size_row;
				lf(currcons);
			}
			__asm__("movb %2,%%ah\n\t"
				"movw %%ax,%1\n\t"
				::"a" (translate[c-32]),
				"m" (*(short *)pos),
				"b" (attr)
				);
			pos += 2;
			x++;
		} else if (c==27)
			;
		else if (c==10 || c==11 || c==12)
			lf(currcons);
		else if (c==13)
			cr(currcons);
		else if (c==ERASE_CHAR(tty))
			del(currcons);
		else if (c==8) {
			if (x) {
				x--;
				pos -= 2;
			}
		} else if (c==9) {
			ht(currcons);
		} else if (c==7)
			;
	  	else if (c == 14)
	  		;
	  	else if (c == 15)
			;
	}
	set_cursor(currcons);
}

void con_init(void)
{
	unsigned char a;
	char * display_desc;
	char * display_ptr;
	int currcons;
	int base, term;
	int video_memory;
	int i, count;
	unsigned short tmp;
	
	fg_console = 0;
	NR_CONSOLES = 0;
	video_num_columns = 80;
	video_mem_base = 0xB8000;
	video_mem_term = 0xC0000;
	video_size_row = video_num_columns * 2;
	video_num_lines = 25;
	video_port_reg = 0x3d4;
	video_port_val = 0x3d5;

	translations =	/* normal 7-bit ascii */
	" !\"#$%&'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmnopqrstuvwxyz{|}~ ";
	
	video_erase_char = 0x0720;
	display_desc = "EGAc";
	currcons = 0;
	video_memory = video_mem_term - video_mem_base;
	NR_CONSOLES = video_memory / (video_num_lines * video_size_row);

	if (NR_CONSOLES > MAX_CONSOLES)
		NR_CONSOLES = MAX_CONSOLES;
	if (!NR_CONSOLES)
		NR_CONSOLES = 1;
	video_memory /= NR_CONSOLES;

	display_ptr = ((char *)video_mem_base) + video_size_row * 2 - 60;
	while (*display_desc)
	{
		*display_ptr++ = *display_desc++;
		display_ptr++;
	}

	base = origin = video_mem_start = video_mem_base;
	term = video_mem_end = base + video_memory;
	scr_end = video_mem_start + video_num_lines * video_size_row;
	top = 0;
	bottom = video_num_lines;
	attr = 0x07;
	translate = translations;

	tmp = get_init_pos();
	y = tmp / 80;
	x = tmp % 80;
	pos = video_mem_base + y * video_size_row + (x << 1);

	for (currcons = 1; currcons < NR_CONSOLES; currcons++)
	{		
		vc_cons[currcons] = vc_cons[0];	
		base += video_memory;
		origin = video_mem_start = base;
		scr_end = origin + video_num_lines * video_size_row;
		term += video_memory;
		video_mem_end = term;
		gotoxy(currcons, 0, 0);
	}
	update_screen();
}

void update_screen(void)
{
	set_origin(fg_console);
	set_cursor(fg_console);
}

extern void console_print(const char * b)
{
	int currcons = fg_console;
	char c;

	while (c = *(b++)) {
		if (c == 10) {
			cr(currcons);
			lf(currcons);
			continue;
		}
		if (c == 13) {
			cr(currcons);
			continue;
		}
		if (c == 9)
		{
			ht(currcons);
			continue;
		}
		if (x>=video_num_columns) {
			x -= video_num_columns;
			pos -= video_size_row;
			lf(currcons);
		}
		__asm__("movb %2,%%ah\n\t"
			"movw %%ax,%1\n\t"
			::"a" (c),
			"m" (*(short *)pos),
			"b" (attr)
			);
		pos += 2;
		x++;
	}
	set_cursor(currcons);
}














