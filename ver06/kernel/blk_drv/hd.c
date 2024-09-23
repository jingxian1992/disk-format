#include <stdio.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <string.h>

#define MAJOR_NR 3
#include <linux/blk.h>

/* Max read/write errors/sector */
#define MAX_ERRORS	7

static void recal_intr(void);
static void bad_rw_intr(void);
static void do_hd_request(void);

struct hd_info_struct hd_info[MAX_HD];
static int NR_HD;
int hd_timeout;
void (* do_hd)(void);

struct hd_struct
{
	int start_sect;
	int nr_sects;
};

static struct hd_struct hd[5 * MAX_HD];
static int hd_sizes[5 * MAX_HD];
static int recalibrate;
static int reset;

extern int sys_print_hd_info(int i)
{
	printk("hard disk %d infomation:\n", i);
	printk("cylender amount: %d\n", hd_info[i].cyl);
	printk("head amount: %d\n", hd_info[i].head);
	printk("start reduce current cylender number: %d\n", hd_info[i].rcur_cyl);
	printk("write pre-compensation cylender number: %d\n", hd_info[i].wpcom);
	printk("biggest ECC length: %d\n", hd_info[i].ECC_len);
	printk("control byte: 0x%02X\n", hd_info[i].ctl);
	printk("standard timeout value: 0x%02X\n", hd_info[i].std_timeout);
	printk("formattint timeout value: 0x%02X\n", hd_info[i].fmt_timeout);
	printk("check timeout value: 0x%02X\n", hd_info[i].check_timeout);
	printk("head loading(stop) cylender number: %d\n", hd_info[i].lzone);
	printk("sector amount per cylender: %d\n", hd_info[i].sect);
	printk("preserve byte: 0x%02X\n", hd_info[i].preserve_byte);
	printk("\n");
	
	return 100;
}

static void create_rootdir(int index)
{
	int sector_num_lba, block_nr;
	unsigned char * disk_buffer;
	struct d_super_block * psuper_block;
	struct d_inode * pinode;
	struct dir_entry * ptr_dir_entry;
	int root_dir_contents_blocknr;
	int j;
	char * de_name;
	
	if (index != 0 && index != 1)
	{
		printk("we only support two disk.\n");
		panic("create root directary error!!!\n");
	}
	else;
	
	block_nr = 1;
	sector_num_lba = block_nr * 2;
	disk_buffer = (void *)DISK_BUFFER;
	psuper_block = (struct d_super_block *)disk_buffer;
	pinode = (struct d_inode *)disk_buffer;
	ptr_dir_entry = (struct dir_entry *)disk_buffer;
	
	printk("read super block.  block No: %d\tsector No: %d\n", block_nr, sector_num_lba);
	
	//读盘以后，disk_buffer存储着超级块的内容。
	if (!index)
	{
		read_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		read_second_disk(sector_num_lba, disk_buffer);
	}
	
	/***************************************
		在block_nr变量里加载起始的i节点表块号，这个块里面的第1个
	  i节点，就是根节点了。在root_dir_contents_blocknr变量里加载第1
	  个数据区的块号。这一块里面，是根目录的第1块的内容。
						****************************************/
	block_nr = (2 + psuper_block->s_imap_blocks + psuper_block->s_zmap_blocks);
	sector_num_lba = block_nr * 2;
	root_dir_contents_blocknr = psuper_block->s_firstdatazone;
	
	printk("prepare to read the first inode table zone. block No: %d\tsector No: %d\n", block_nr, sector_num_lba);
		
	//读盘以后，disk_buffer存储着根节点结构体信息。
	if (!index)
	{
		read_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		read_second_disk(sector_num_lba, disk_buffer);
	}
	
	//设置根节点的各个字段值
	pinode->i_mode = S_IFDIR | 0777;	//文件类型为目录，权限为任何人可读写，可执行。
	pinode->i_uid = 0;		//root用户的uid应该是0
	pinode->i_size = sizeof(struct dir_entry) * 2;  //只设置两个目录，【.】和【..】
	pinode->i_time = CURRENT_TIME();
	pinode->i_gid = 0;		//root用户的gid应该是0。
	pinode->i_nlinks = 2;	//初始创建的目录，链接数为2。
	pinode->i_zone[0] = root_dir_contents_blocknr;	//在第一个数据块中存储根目录内容
	for (j = 1; j < 9; j++)		//除了第1块，其余块号设置为0，表示未使用。
	{
		pinode->i_zone[j] = 0;
	}

	/*******************************
		设置好了根节点以后，要将根节点所在的缓冲区内容，写入磁盘，
	  以保存我方的设置内容。
						******************************/
	if (!index)
	{
		write_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		write_second_disk(sector_num_lba, disk_buffer);
	}
	
	/***********************
		让sector_num_lba指向起始数据区，也就是根目录的第一块。
									**********************/
	block_nr = root_dir_contents_blocknr;
	sector_num_lba = block_nr * 2;
	printk("first data zone block No: %d\tsector No: %d\n", block_nr, sector_num_lba);
	
	//读盘以后，disk_buffer保存着根目录的第一块内容。
	if (!index)
	{
		read_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		read_second_disk(sector_num_lba, disk_buffer);
	}
	memset(disk_buffer, 0, BLOCK_SIZE);		//对新读出的磁盘块的内容作清零处理。
	
	/*************************************
		以下内容，用于在根目录的第一块所在的缓冲区里面，建立【.】
	  和【..】两个目录项。
					**************************************/
	ptr_dir_entry->inode = ROOT_INO;
	de_name = ptr_dir_entry->name;
	strcpy(de_name, ".");		//strcpy函数是会复制字符串末尾的'\0'的哦。
	printk("directary name: %s\tinode No: %d\n", de_name, ptr_dir_entry->inode);
	
	ptr_dir_entry++;
	ptr_dir_entry->inode = ROOT_INO;
	de_name = ptr_dir_entry->name;		//重新设置，否则de_name指向前一项的
										//相同字段。
	strcpy(de_name, "..");
	printk("directary name: %s\tinode No: %d\n", de_name, ptr_dir_entry->inode);
	/***************************
		个人觉得，标准库 <string.h> 里面的字符串操作函数非常好用。
	  以上的操作，已经设置好了根目录的两个目录项了，接下来，就可以写
	  盘，以保存我方的设置了。
								**************************/
	if (!index)
	{
		write_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		write_second_disk(sector_num_lba, disk_buffer);
	}
	
	printk("I hope that, this functiopn can work well.\n");
	return;
}

extern int sys_format_hd(int index)
{
	struct partition * part;
	int heads, cyls, sectors;
	int sector_total, block_total;
	int end_head, end_cyl, end_sect, end_lba_sect;
	int tmp, sector_num_lba;
	unsigned char * disk_buffer;
	struct d_super_block * super_block;
	int block_num, bitmap_num, logzone_num;
	int inode_block_num;
	int s_imap_blocks, s_zmap_blocks;
	int s_ninodes, s_nzones;
	int bit_odd, byte_odd, num_01, num_02;
	int j;
	unsigned char bitmap_odd_byte[8] = {
		0xFE, 0xFC, 0xF8, 0xF0,
		0xE0, 0xC0, 0x80, 0x00
	};
	
	if (index != 0 && index != 1)
	{
		printk("we only support two disk.\n");
		panic("hard disk format error!!!\n");
	}
	else;
	
	printk("format hard disk %d\tfile system name: minix 1.0\n", index);
	
	sector_num_lba = 0;
	disk_buffer = (unsigned char *)DISK_BUFFER;
	part = (struct partition *)(disk_buffer + 0x1BE);
	memset(disk_buffer, 0, 1024);	
	
	heads = hd_info[index].head;
	cyls = hd_info[index].cyl;
	sectors = hd_info[index].sect;
	sector_total = heads * cyls * sectors;
	block_total = sector_total >> 1;
	block_total &= 0xFFFFFFC0;
	
	if (block_total < 360)
	{
		panic("disk size too little.\n");
	}
	else if (block_total > MAX_LOG_ZONE_AMOUNT)
	{
		block_total = MAX_LOG_ZONE_AMOUNT;
	}
	else;
	
	printk("effective disk block amount: %d (one blocak is 1024 bytes.)\n", block_total);
	
	sector_total = block_total << 1;
	end_lba_sect = sector_total - 1;
	end_sect = (end_lba_sect % sectors) + 1;
	tmp = end_lba_sect / sectors;
	end_head = tmp % heads;
	end_cyl = tmp/heads;	
	
	printk("end LBA No: %d\tsector total amount: %d\n", end_lba_sect, sector_total);
	
	part->boot_ind = 0x00;
	part->head = 0;
	part->sector = 1;
	part->cyl = 0;
	part->sys_ind = 0x80;   //minix---0x80
	part->end_head = end_head;
	part->end_sector = (end_sect & 0x3F) | ((end_cyl >> 2) & 0xC0);
	part->end_cyl = end_cyl & 0xFF;
	part->start_sect = 0;
	part->nr_sects = sector_total;
	disk_buffer[510] = 0x55;
	disk_buffer[511] = 0xAA;
	
	if (!index)
	{
		write_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		write_second_disk(sector_num_lba, disk_buffer);
	}
	sector_num_lba += 2;
	

// super block formating
	memset(disk_buffer, 0, 1024);
	super_block = disk_buffer;

	tmp = block_total;	//有多少个磁盘块，就有多少个i节点。
	/*********************************************
	    如果i节点数大于8个i节点位图所能表示的最大值，则令i节点数等于
	  8个i节点位图所能表示的最大值。
	**************************************************/
	if (tmp > MAX_LOG_ZONE_AMOUNT - 1)
	{
		tmp = MAX_LOG_ZONE_AMOUNT - 1;
	}

	super_block->s_ninodes = tmp;	//设置i节点数目
	s_ninodes = super_block->s_ninodes;
	
	tmp = tmp/8192 + 1;		//8191个节点占用1个位图块，8192个节点要占用第2个位图块
	super_block->s_imap_blocks = tmp;
	s_imap_blocks = super_block->s_imap_blocks;	//保存i节点位图块数

	num_01 = BLOCK_SIZE/(sizeof(struct d_inode));	//1个磁盘块容纳的i节点数
	num_02 = num_01 - 1;		//num_02变量用于下一行的向上取整
	inode_block_num = (super_block->s_ninodes + num_02)/num_01;
	/*********************************************************
	    inode_block_num 表示i节点表占用的磁盘块数目。不涉及位图块的位0
	    不用的问题，所以，直接向上取整就行了	
	***********************************************************/
	printk("inode table block amount: %d\n", inode_block_num);

	super_block->s_log_zone_size = BLOCK_SIZE_BITS;
	super_block->s_max_size = 7 * 512 + 512 * 512 + 512 * 512 * 512;
	super_block->s_magic = SUPER_MAGIC;
	
	/********  这几行代码，用于计算逻辑块位图块数和数据区的逻辑块数目。在纸上
	    研究，演算和测试了好一会儿，才整理出来的。  *********/
	block_num = block_total - 2 - s_imap_blocks - inode_block_num;
	bitmap_num = block_num/8192 + 1;
	logzone_num = block_num - bitmap_num;
	bitmap_num = logzone_num/8192 + 1;
	logzone_num = block_num - bitmap_num;
	super_block->s_nzones = logzone_num;
	s_nzones = super_block->s_nzones;
	super_block->s_zmap_blocks = bitmap_num;
	s_zmap_blocks = super_block->s_zmap_blocks;  //保存逻辑块位图块数
	/**********************************************
	      在minix 1.0文件系统里面，i节点位图块数和逻辑块位图块数，分别要求小于
	   或等于8。关于这一点，我是在格式化引导块的时候，检测了总的磁盘块数目，确
	   保整个磁盘的磁盘块数不大于8个位图块多能表示的最大数目加1。数据区的逻辑块数，
	   肯定会小于整个磁盘的总块数。因此，super_block->s_zmap_blocks肯定是
	   符合范围规定的。直接在引导块格式化时，设置最大数，图个省事，偷个懒了。
	      至于i节点数，前面已经讲述了。 
	   ******************************************************/
	
	
	/*  对于一个 minix 1.0 文件系统的分区而言，前两块是引导块和超级块，接
	  下来是i节点位图块，逻辑块位图块，i节点表。i节点之后，便是数据区了。
	  也就是说，i节点表之后的第一块所在的块号，便是 s_firstdatazone   */
	super_block->s_firstdatazone = 2 + super_block->s_imap_blocks + super_block->s_zmap_blocks + inode_block_num;

	printk("inode amount: %d\tinode bitmap block amount: %d\n", super_block->s_ninodes, super_block->s_imap_blocks);
	printk("logic zone amount: %d\tlogic zone bitmap block amount: %d\n", super_block->s_nzones, super_block->s_zmap_blocks);
	printk("file system magic number: 0x%X\tmax file size: 0x%X bytes\n", super_block->s_magic, super_block->s_max_size);
	printk("first data zone block No: %d\n", super_block->s_firstdatazone);

	if (!index)
	{
		write_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		write_second_disk(sector_num_lba, disk_buffer);
	}
	sector_num_lba += 2;
	
// inode bitmap formating
	memset(disk_buffer, 0, 1024);
	disk_buffer[0] |= 3;
	/******************************************************
		要将第一块i节点位图块的位0置1，这是因为，实际的i节点表所在的磁盘区
	  域，第一个i节点的编号并不是0，而是1。也就是，第一块i节点位图位0所表
	  示的0号i节点，实际上并不存在。而位1所表示的1号i节点则是真实存在着的。
	  	要将第一块i节点位图块的位1置1，这是因为，第一个i节点位图块的位1所
	  表示的i节点，必定会用来创建根节点，以创建根目录，会被固定地分配出去。
	****************************************************/
	
	if (s_imap_blocks > 1)
	{
	/** 如果i节点位图块数大于1，则先写第1块，然后将缓冲区清0，用清零的缓冲区
	   去接着写第2块，第3块，直到写完倒数第2块。最后一块留着，先不写 */
		if (!index)
		{
			write_first_disk(sector_num_lba, disk_buffer);
		}
		else
		{
			write_second_disk(sector_num_lba, disk_buffer);
		}
		sector_num_lba += 2;
		
		memset(disk_buffer, 0, 1024);
		for (j = 1; j < s_imap_blocks - 1; j++)
		{
			if (!index)
			{
				write_first_disk(sector_num_lba, disk_buffer);
			}
			else
			{
				write_second_disk(sector_num_lba, disk_buffer);
			}
			sector_num_lba += 2;
		}
	}
	else;
	
	/*******************************************
	    num_01用于计算最后一个i节点会在最后一个i节点位图块里面，位于哪个位。
	  一个i节点位图一般是可以容纳8192个位。但是第1个i节点位图的位0不用，仅能
	  容纳8191个有效位。这样的话，如果有8192个i节点，则会占用第2个i节点位图，
	  且占用第2个i节点位图块的位0。在计算公式里面，直接取余数就可以了，不用加1
	  和减1。
	  						*********************************************/
	num_01 = s_ninodes % 8192;
	byte_odd = num_01/8;
	bit_odd = byte_odd & 7;
	disk_buffer[byte_odd++] = bitmap_odd_byte[bit_odd];
		
	for (j = byte_odd; j < 1024; j++)
	{
		disk_buffer[j] = 0xFF;
	}
		
	if (!index)
	{
		write_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		write_second_disk(sector_num_lba, disk_buffer);
	}		
	sector_num_lba += 2;
	
	
// logzone bitmap formating
	memset(disk_buffer, 0, 1024);
	disk_buffer[0] |= 3;
	/******************************************************
		要将第一块逻辑块位图的位0置1，这是因为，实际的数据区磁盘块，第一块
	  的编号并不是0，而是1。也就是，位0所表示的第0块，实际上并不存在。而位
	  1则是真实存在着的。
	  	要将第一块逻辑块位图的位1置1，这是因为，第一个逻辑块位图的位1所表示
	   的磁盘块，必定会用来创建根目录，会被固定地分配出去。
	****************************************************/

	if (s_zmap_blocks > 1)
	{
	/** 如果逻辑块位图块数大于1，则先写第1块。然后将缓冲区清0，用清零的缓冲区
	   去接着写第2块，第3块，直到写完倒数第2块。最后一块留着，先不写 */
		if (!index)
		{
			write_first_disk(sector_num_lba, disk_buffer);
		}
		else
		{
			write_second_disk(sector_num_lba, disk_buffer);
		}
		sector_num_lba += 2;
		
		memset(disk_buffer, 0, 1024);
		for (j = 1; j < s_zmap_blocks - 1; j++)
		{
			if (!index)
			{
				write_first_disk(sector_num_lba, disk_buffer);
			}
			else
			{
				write_second_disk(sector_num_lba, disk_buffer);
			}
			sector_num_lba += 2;
		}
	}
	else;
	
	/*******************************************
	    num_01用于计算最后一个逻辑块会在最后一个逻辑块位图块里面，位于哪个位。
	  一个逻辑块位图一般是可以容纳8192个位。但是第1个逻辑块位图的位0不用，仅能
	  容纳8191个有效位。这样的话，如果有8192个逻辑块，则会占用第2个逻辑块位图，
	  且占用第2个逻辑块位图块的位0。在计算公式里面，直接取余数就可以了，不用加1
	  和减1。
	  						*********************************************/
	num_01 = s_nzones % 8192;
	byte_odd = num_01/8;
	bit_odd = byte_odd & 7;
	disk_buffer[byte_odd++] = bitmap_odd_byte[bit_odd];

	for (j = byte_odd; j < 1024; j++)
	{
		disk_buffer[j] = 0xFF;
	}
		
	if (!index)
	{
		write_first_disk(sector_num_lba, disk_buffer);
	}
	else
	{
		write_second_disk(sector_num_lba, disk_buffer);
	}		
	sector_num_lba += 2;

// inode table formating
	memset(disk_buffer, 0, 1024);
	for (j = 0; j < inode_block_num; j++)
	{
		if (!index)
		{
			write_first_disk(sector_num_lba, disk_buffer);
		}
		else
		{
			write_second_disk(sector_num_lba, disk_buffer);
		}				
	}
	
	printk("hard disk %d formating is done.\n", index);
	printk("I hope that this function can work well.\n");
	
	create_rootdir(index);
	
	return 100;
}

static void SET_INTR(void (* x)(void))
{
	do_hd = x;
	hd_timeout = 200;
}

static int DEVICE_NR(int device)
{
	int res;
	res = MINOR(device) / 5;
	return res;
}

static inline void unlock_buffer(struct buffer_head * bh)
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
		unlock_buffer(CURRENT->bh);
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

static void hd_out(unsigned int drive,unsigned int nsect,unsigned int sect,
		unsigned int head,unsigned int cyl,unsigned int cmd,
		void (*intr_addr)(void))
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

void unexpected_hd_interrupt(void)
{
	printk("Unexpected HD interrupt\n\r");
	reset = 1;
	do_hd_request();
}

static void bad_rw_intr(void)
{
	if (++CURRENT->errors >= MAX_ERRORS)
		end_request(0);
	if (CURRENT->errors > MAX_ERRORS/2)
		reset = 1;
}

static void read_intr(void)
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

static void write_intr(void)
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

static void recal_intr(void)
{
	if (win_result())
		bad_rw_intr();
	do_hd_request();
}

void hd_times_out(void)
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

void do_hd_request(void)
{
	int i,r;
	unsigned int block,dev;
	unsigned int sec,head,cyl;
	unsigned int nsect;
	unsigned int hd_info_sect, hd_info_head;

	hd_info_sect = hd_info[dev].sect;
	hd_info_head = hd_info[dev].head;

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
	block = CURRENT->sector;
	if (dev >= 5*NR_HD || block+2 > hd[dev].nr_sects) {
		end_request(0);
		goto repeat;
	}
	block += hd[dev].start_sect;
	dev /= 5;
	__asm__("divl %4":"=a" (block),"=d" (sec):"0" (block),"1" (0),
		"r" (hd_info_sect));
	__asm__("divl %4":"=a" (cyl),"=d" (head):"0" (block),"1" (0),
		"r" (hd_info_head));
	sec++;
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
		hd_out(dev,nsect,sec,head,cyl,WIN_WRITE,&write_intr);
		for(i=0 ; i<10000 && !(r=inb_p(HD_STATUS)&DRQ_STAT) ; i++)
			/* nothing */ ;
		if (!r) {
			bad_rw_intr();
			goto repeat;
		}
		port_write(HD_DATA,CURRENT->buffer,256);
	} else if (CURRENT->cmd == READ) {
		hd_out(dev,nsect,sec,head,cyl,WIN_READ,&read_intr);
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
	
	memcpy(hd_info, (void *)HD_PARAM_TABLE, sizeof(hd_info));
	NR_HD = MAX_HD;
	
	hd_info[0].cyl = HD0_CYL;
	hd_info[0].head = HD0_HEAD;
	hd_info[0].sect = HD0_SECTOR;
	hd_info[0].wpcom = HD0_WPCOM;
	hd_info[0].ctl = HD0_CTL;
	hd_info[0].lzone = HD0_LZONE;
	
	for (i = 0; i < (5 * MAX_HD); i++)
	{
		hd[i].start_sect = 0;
		hd[i].nr_sects = 0;
	}
	
	for (i = 0; i < (5 * MAX_HD); i++)
	{
		hd_sizes[i] = 0;
	}

	recalibrate = 0;
	reset = 0;	
	hd_timeout = 0;
	do_hd = NULL;
	recalibrate = 0;
	reset = 0;

//	blk_dev[MAJOR_NR].request_fn = do_hd_request;
//	set_intr_gate(0x2E,&hd_interrupt);
//	outb_p(inb_p(0x21)&0xfb,0x21);
//	outb(inb_p(0xA1)&0xbf,0xA1);
}
