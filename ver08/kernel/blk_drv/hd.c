#include <stdio.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <sys/time.h>
#include <linux/config.h>

#define MAJOR_NR 3
#include <linux/blk.h>

/* Max read/write errors/sector */
#define MAX_ERRORS	7

struct hd_struct hd[5 * MAX_HD];
int hd_sizes[5 * MAX_HD];
int callable;
int recalibrate;
int reset;
int NR_HD;
int hd_timeout;
void (* do_hd)(void);
int sys_load_flags;

unsigned int first_part_blockno;
unsigned int first_part_sectno;

static void SET_INTR(void (* x)(void))
{
	do_hd = x;
}

static int DEVICE_NR(int device)
{
	int res;
	res = MINOR(device) / 5;
	return res;
}

static inline void unlock_buffer_hd(struct buffer_head * bh)
{
	if (!bh->b_lock)
		printk("harddisk: free buffer being unlocked\n");
	bh->b_lock=0;
	wake_up(&bh->b_wait);
}

static inline void end_request(int uptodate)
{
	if (CURRENT->bh) {
		CURRENT->bh->b_uptodate = uptodate;
		unlock_buffer_hd(CURRENT->bh);
	}
	if (!uptodate) {
		printk("harddisk I/O error\n\r");
		printk("dev %04x, block %d\n\r",CURRENT->dev,
			CURRENT->bh->b_blocknr);
	}
	wake_up(&CURRENT->waiting);
	wake_up(&wait_for_request);
	CURRENT->dev = -1;
	CURRENT = CURRENT->next;
}
 
static void CLEAR_DEVICE_TIMEOUT(void)
{
	hd_timeout = 0;
}

static void CLEAR_DEVICE_INTR(void)
{
	do_hd = NULL;
}

static void port_read(int port, void * buf, int nr)
{
__asm__("cld;rep;insw"::"d" (port),"D" (buf),"c" (nr));
}

static void port_write(int port, void * buf, int nr)
{
__asm__("cld;rep;outsw"::"d" (port),"S" (buf),"c" (nr));
}

static int controller_ready(void)
{
	int retries = 100000;

	while (--retries && (inb_p(HD_STATUS)&0xc0)!=0x40);
	return (retries);
}

static int win_result(void)
{
	int i=inb_p(HD_STATUS);

	if ((i & (BUSY_STAT | READY_STAT | WRERR_STAT | SEEK_STAT | ERR_STAT))
		== (READY_STAT | SEEK_STAT))
		return(0); /* ok */
	if (i&1) i=inb(HD_ERROR);
	return (1);
}

extern void hd_out(int drive, int nsect, int sect,
  int head, int cyl, int cmd, void (*intr_addr)(void))
{
	register int port asm("dx");

	if (drive>1 || head>15)
		panic("Trying to write bad sector");
	if (!controller_ready())
		panic("HD controller not ready");
	SET_INTR(intr_addr);
	outb_p(hd_info[drive].ctl,HD_CMD);
	port=HD_DATA;
	outb_p(hd_info[drive].wpcom>>2,++port);
	outb_p(nsect,++port);
	outb_p(sect,++port);
	outb_p(cyl,++port);
	outb_p(cyl>>8,++port);
	outb_p(0xA0|(drive<<4)|head,++port);
	outb(cmd,++port);
}

extern void hd_out_LBA(int drive, int nsect, int sector,
  int cmd, void (*intr_addr)(void))
{
	unsigned short port;
	unsigned char data, status_info;

	if (drive>1)
		panic("Trying to write bad sector");
	if (!controller_ready())
		panic("HD controller not ready");

	SET_INTR(intr_addr);

	port = HD_NSECTOR;
	data = nsect;
	outb_p(data, port++);		// 向0x1F2写入本次操作的扇区数
	
	data = sector & 0xFF;
	outb_p(data, port++);   // 向0x1F3写入扇区号低8位
	
	data = (sector >> 8) & 0xFF;
	outb_p(data, port++);   // 向0x1F4写入扇区号的位8……位15
	
	data = (sector >> 16) & 0xFF;
	outb_p(data, port++);   // 向0x1F5写入扇区号的位16……位23

	/*******************************************************
		扇区号的位24……位27放在data的低4位。
	  位5……为7若固定设置为111，则表示采用LBA寻址方式。
	  位4为0，表示操作主盘，为1表示操作从盘。
	  几个部分的数据进行位与操作，就可以拼出目标值了。		
		*******************************************************/
	data = (sector >> 24) & 0x0F;
	data |= 0xE0;
	data |= (drive << 4);
	outb_p(data, port++);

	data = cmd;
	outb_p(data, port);

	return;
}

static int drive_busy(void)
{
	unsigned int i;
	unsigned char c;

	for (i = 0; i < 50000; i++) {
		c = inb_p(HD_STATUS);
		c &= (BUSY_STAT | READY_STAT | SEEK_STAT);
		if (c == (READY_STAT | SEEK_STAT))
			return 0;
	}
	printk("HD controller times out\n\r");
	return(1);
}

static void reset_controller(void)
{
	int	i;

	outb(4,HD_CMD);
	for(i = 0; i < 1000; i++) nop();
	outb(hd_info[0].ctl & 0x0f ,HD_CMD);
	if (drive_busy())
		printk("HD-controller still busy\n\r");
	if ((i = inb(HD_ERROR)) != 1)
		printk("HD-controller reset failed: %02x\n\r",i);
}

static void reset_hd(void)
{
	static int i;

repeat:
	if (reset) {
		reset = 0;
		i = -1;
		reset_controller();
	} else if (win_result()) {
		bad_rw_intr();
		if (reset)
			goto repeat;
	}
	i++;
	if (i < NR_HD) {
		hd_out(i,hd_info[i].sect,hd_info[i].sect,hd_info[i].head-1,
			hd_info[i].cyl,WIN_SPECIFY,&reset_hd);
	} else
		do_hd_request();
}

extern void unexpected_hd_interrupt(void)
{
	if (sys_load_flags)
	{
		return;
	}
	else;
	
	printk("Unexpected HD interrupt\n\r");
	reset = 1;
	do_hd_request();
}

extern void hd_times_out(void)
{
	if (!CURRENT)
		return;
	printk("HD timeout");
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	SET_INTR(NULL);
	reset = 1;
	do_hd_request();
}

extern void bad_rw_intr(void)
{
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

extern void read_intr(void)
{
	if (win_result()) {
		bad_rw_intr();
		do_hd_request();
		return;
	}
	port_read(HD_DATA,CURRENT->buffer,256);
	CURRENT->errors = 0;
	CURRENT->buffer += 512;
	CURRENT->sector++;
	if (--CURRENT->nr_sectors) {
		SET_INTR(&read_intr);
		return;
	}
	end_request(1);
	do_hd_request();
}

extern void write_intr(void)
{
	if (win_result()) {
		bad_rw_intr();
		do_hd_request();
		return;
	}
	if (--CURRENT->nr_sectors) {
		CURRENT->sector++;
		CURRENT->buffer += 512;
		SET_INTR(&write_intr);
		port_write(HD_DATA,CURRENT->buffer,256);
		return;
	}
	end_request(1);
	do_hd_request();
}

extern void recal_intr(void)
{
	if (win_result())
		bad_rw_intr();
	do_hd_request();
}

void do_hd_request(void)
{
	int i,r;
	int sector,dev;
	int nsect;

repeat:
	if (!CURRENT) {
		CLEAR_DEVICE_INTR();
		CLEAR_DEVICE_TIMEOUT();
		return;
	}

	if (MAJOR(CURRENT->dev) != MAJOR_NR)
		panic("harddisk: request list destroyed");

	if (CURRENT->bh) {
		if (!CURRENT->bh->b_lock)
			panic("harddisk: block not locked");
	}

	dev = MINOR(CURRENT->dev);
	sector = CURRENT->sector;

	if (dev >= 5*NR_HD || sector+2 >= hd[dev].nr_sects) {
		end_request(0);
		goto repeat;
	}

	sector += hd[dev].start_sect;
	dev /= 5;
	nsect = CURRENT->nr_sectors;

	if (reset) {
		recalibrate = 1;
		reset_hd();
		return;
	}
	if (recalibrate) {
		recalibrate = 0;
		hd_out(dev,hd_info[CURRENT_DEV].sect,0,0,0,
			WIN_RESTORE,&recal_intr);
		return;
	}	
	if (CURRENT->cmd == WRITE) {
		hd_out_LBA(dev,nsect,sector,WIN_WRITE,&write_intr);
		for(i=0 ; i<10000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		if (!r) {
			bad_rw_intr();
			goto repeat;
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	} else if (CURRENT->cmd == READ) {
		hd_out_LBA(dev,nsect,sector,WIN_READ,&read_intr);
	} else
		panic("unknown hd-command");
}

void read_first_disk_base(unsigned int sector_num_lba, unsigned short * buffer)
{
	unsigned short port;
	unsigned char data, status_info;
	
	port = 0x1f2;
	data = 1;
	outb_p(data, port++);  // 向0x1F2写入本次操作的扇区数
	
	data = sector_num_lba & 0xFF;
	outb_p(data, port++);   // 向0x1F3写入扇区号低8位
	
	data = (sector_num_lba >> 8) & 0xFF;
	outb_p(data, port++);   // 向0x1F4写入扇区号的位8……位15
	
	data = (sector_num_lba >> 16) & 0xFF;
	outb_p(data, port++);   // 向0x1F5写入扇区号的位16……位23
	
	data = (sector_num_lba >> 24) & 0x0F;    // 扇区号的位24……位27放在data的低4位
	data |= 0xE0;     // data的高4位，设置为 0B1110，表示的是LBA操作方式。
	outb_p(data, port++);    // 将组建好的data值发送到端口0x1F6里面。
	
	data = 0x20;
	outb_p(data, port);   // 向)x1f7写入0x20命令。0x30表示【可重试读扇区】
	
	do
	{
		status_info = inb_p(port) & 0x88;
	} while (status_info != 0x08);
		
	port = 0x1F0;
	port_read(port, buffer, 256);
}

void read_second_disk_base(unsigned int sector_num_lba, unsigned short * buffer)
{
	unsigned short port;
	unsigned char data, status_info;
	
	port = 0x1f2;
	data = 1;
	outb_p(data, port++);  // 向0x1F2写入本次操作的扇区数
	
	data = sector_num_lba & 0xFF;
	outb_p(data, port++);   // 向0x1F3写入扇区号低8位
	
	data = (sector_num_lba >> 8) & 0xFF;
	outb_p(data, port++);   // 向0x1F4写入扇区号的位8……位15
	
	data = (sector_num_lba >> 16) & 0xFF;
	outb_p(data, port++);   // 向0x1F5写入扇区号的位16……位23
	
	data = (sector_num_lba >> 24) & 0x0F;    // 扇区号的位24……位27放在data的低4位
	data |= 0xF0;     // data的高4位，设置为 0B1111，表示的是LBA操作方式。
	outb_p(data, port++);    // 将组建好的data值发送到端口0x1F6里面。
	
	data = 0x20;
	outb_p(data, port);   // 向)x1f7写入0x20命令。0x30表示【可重试读扇区】
	
	do
	{
		status_info = inb_p(port) & 0x88;
	} while (status_info != 0x08);
		
	port = 0x1F0;
	port_read(port, buffer, 256);
}

void write_first_disk_base(unsigned int sector_num_lba, unsigned short * buffer)
{
	unsigned short port, data_info;
	unsigned char data, status_info;
	int i;
	
	port = 0x1f2;
	data = 1;
	outb_p(data, port++);  // 向0x1F2写入本次操作的扇区数
	
	data = sector_num_lba & 0xFF;
	outb_p(data, port++);   // 向0x1F3写入扇区号低8位
	
	data = (sector_num_lba >> 8) & 0xFF;
	outb_p(data, port++);   // 向0x1F4写入扇区号的位8……位15
	
	data = (sector_num_lba >> 16) & 0xFF;
	outb_p(data, port++);   // 向0x1F5写入扇区号的位16……位23
	
	data = (sector_num_lba >> 24) & 0x0F;    // 扇区号的位24……位27放在data的低4位
	data |= 0xE0;     // data的高4位，设置为 0B1110，表示的是LBA操作方式。
	outb_p(data, port++);    // 将组建好的data值发送到端口0x1F6里面。
	
	data = 0x30;
	outb_p(data, port);   // 向)x1f7写入0x30命令。0x30表示【可重试写扇区】
	
	for (i = 0; i < 10000; i++)
	{
		;
	}
		
	port = 0x1F0;
	port_write(port, buffer, 256);
	
	for (i = 0; i < 10000; i++)
	{
		;
	}

	port = 0x1F7;
	do
	{
		status_info = inb_p(port);
		status_info &= 0x80;
	}while (status_info != 0x00);
}

void write_second_disk_base(unsigned int sector_num_lba, unsigned short * buffer)
{
	unsigned short port, data_info;
	unsigned char data, status_info;
	int i;
	
	port = 0x1f2;
	data = 1;
	outb_p(data, port++);  // 向0x1F2写入本次操作的扇区数
	
	data = sector_num_lba & 0xFF;
	outb_p(data, port++);   // 向0x1F3写入扇区号低8位
	
	data = (sector_num_lba >> 8) & 0xFF;
	outb_p(data, port++);   // 向0x1F4写入扇区号的位8……位15
	
	data = (sector_num_lba >> 16) & 0xFF;
	outb_p(data, port++);   // 向0x1F5写入扇区号的位16……位23
	
	data = (sector_num_lba >> 24) & 0x0F;    // 扇区号的位24……位27放在data的低4位
	data |= 0xF0;     // data的高4位，设置为 0B1111，表示的是LBA操作方式。
	outb_p(data, port++);    // 将组建好的data值发送到端口0x1F6里面。
	
	data = 0x30;
	outb_p(data, port);   // 向)x1f7写入0x30命令。0x30表示【可重试写扇区】
	
	for (i = 0; i < 10000; i++)
	{
		;
	}
		
	port = 0x1F0;
	port_write(port, buffer, 256);
	
	for (i = 0; i < 10000; i++)
	{
		;
	}

	port = 0x1F7;
	do
	{
		status_info = inb_p(port);
		status_info &= 0x80;
	}while (status_info != 0x00);
}

void read_first_disk(unsigned int sector_num_lba, unsigned short * buffer)
{
	read_first_disk_base(sector_num_lba, buffer);
	
	sector_num_lba++;
	buffer += 256;
	
	read_first_disk_base(sector_num_lba, buffer);
}

void read_second_disk(unsigned int sector_num_lba, unsigned short * buffer)
{
	read_second_disk_base(sector_num_lba, buffer);
	
	sector_num_lba++;
	buffer += 256;
	
	read_second_disk_base(sector_num_lba, buffer);
}

void write_first_disk(unsigned int sector_num_lba, unsigned short * buffer)
{
	write_first_disk_base(sector_num_lba, buffer);
	
	sector_num_lba++;
	buffer += 256;
	
	write_first_disk_base(sector_num_lba, buffer);
}

void write_second_disk(unsigned int sector_num_lba, unsigned short * buffer)
{
	write_second_disk_base(sector_num_lba, buffer);
	
	sector_num_lba++;
	buffer += 256;
	
	write_second_disk_base(sector_num_lba, buffer);
}

void hd_init(void)
{
	int i;
	
	if (!ROOT_DEV && !SWAP_DEV)
	{
		SWAP_DEV = 0;
		ROOT_DEV = 0X0306;
	}
	else;

	if ((!first_part_blockno) || (!first_part_sectno))
	{
		first_part_blockno = FIRST_PART_BLOCKNO;
		first_part_sectno = FIRST_PART_SECTNO;
	}
	
	memcpy(hd_info, (void *)HD_PARAM_TABLE, sizeof(hd_info));
	
	sys_load_flags = 1;
	NR_HD = 0;
	
	hd_info[0].cyl = HD0_CYL;
	hd_info[0].head = HD0_HEAD;
	hd_info[0].sect = HD0_SECTOR;
	hd_info[0].wpcom = HD0_WPCOM;
	hd_info[0].ctl = HD0_CTL;
	hd_info[0].lzone = HD0_LZONE;

/*
	hd_info[1].cyl = HD1_CYL;
	hd_info[1].head = HD1_HEAD;
	hd_info[1].sect = HD1_SECTOR;
	hd_info[1].wpcom = HD1_WPCOM;
	hd_info[1].ctl = HD1_CTL;
	hd_info[1].lzone = HD1_LZONE;	
*/

	for (i = 0; i < (5 * MAX_HD); i++)
	{
		hd[i].start_sect = 0;
		hd[i].nr_sects = 0;
	}
	
	for (i = 0; i < (5 * MAX_HD); i++)
	{
		hd_sizes[i] = 0;
	}

	callable = 1;
	recalibrate = 0;
	reset = 0;	
	hd_timeout = 0;
	do_hd = NULL;

	blk_dev[MAJOR_NR].request_fn = do_hd_request;

	set_intr_gate(0x2E,&hd_interrupt);
	outb_p(inb_p(0x21)&0xfb,0x21);
	outb(inb_p(0xA1)&0xbf,0xA1);	
}
