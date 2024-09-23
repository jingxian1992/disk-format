#include <stdio.h>
#include <linux/sys.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <sys/utsname.h>
#include <asm/segment.h>
#include <string.h>
#include <errno.h>
#include <linux/config.h>
#include <sys/stat.h>

#ifndef MAJOR_NR
#define MAJOR_NR 3
#endif
#include <linux/blk.h>

extern int sys_write_simple(const char * buf, int count)
{
	tty_write(0, buf, count);	
	return 100;
}

extern int sys_print_sys_time(void)
{
	int year, moon, day, hour, minute, second;
	second = CMOS_READ(0);
	minute = CMOS_READ(2);
	hour = CMOS_READ(4);
	day = CMOS_READ(7);
	moon = CMOS_READ(8);
	year = CMOS_READ(9);

	BCD_TO_BIN(second);
	BCD_TO_BIN(minute);
	BCD_TO_BIN(hour);
	BCD_TO_BIN(day);
	BCD_TO_BIN(moon);
	BCD_TO_BIN(year);
	
	year = year + 2000;
	
	printk("\tthe current time is: ");
	printk("%d-%d-%d   %d:%d:%d\n", year, moon, day, hour, minute, second);
	
	return 100;
}

extern int sys_print_sys_info(void)
{
	printk("\tcurrent system name: ");
	printk(thisname.sysname);
	printk("\n\tcurrent node name: ");
	printk(thisname.nodename);
	printk("\n\tcurrent release: ");
	printk(thisname.release);
	printk("\n\tcurrent version number: ");
	printk(thisname.version);
	printk("\n\tcurrent machine: ");
	printk(thisname.machine);
	printk("\n");
	
	return 100;
}

extern int sys_setup(void)
{
	int i,drive;
	unsigned char cmos_disks;
	struct partition *p;
	struct buffer_head * bh;
	int block_nr, sector_num_lba;
	char * buf;

	if (!callable)
		return -1;
	callable = 0;

	if (hd_info[1].cyl)
		NR_HD=2;
	else
		NR_HD=1;

	for (i=0 ; i<NR_HD ; i++) {
		hd[i*5].start_sect = first_part_sectno;
		hd[i*5].nr_sects = hd_info[i].head * hd_info[i].sect * hd_info[i].cyl;
	}

	/*
		We querry CMOS about hard disks : it could be that 
		we have a SCSI/ESDI/etc controller that is BIOS
		compatable with ST-506, and thus showing up in our
		BIOS table, but not register compatable, and therefore
		not present in CMOS.

		Furthurmore, we will assume that our ST-506 drives
		<if any> are the primary drives in the system, and 
		the ones reflected as drive 1 or 2.

		The first drive is stored in the high nibble of CMOS
		byte 0x12, the second in the low nibble.  This will be
		either a 4 bit drive type or 0xf indicating use byte 0x19 
		for an 8 bit type, drive 1, 0x1a for drive 2 in CMOS.

		Needless to say, a non-zero value means we have 
		an AT controller hard disk for that drive.

	*/

	if ((cmos_disks = CMOS_READ(0x12)) & 0xf0)
	{
		if (cmos_disks & 0x0f)
			NR_HD = 2;
		else
			NR_HD = 1;
	}
	else
		NR_HD = 0;
	for (i = NR_HD ; i < 2 ; i++) {
		hd[i*5].start_sect = 0;
		hd[i*5].nr_sects = 0;
	}

	for (drive=0 ; drive<1 ; drive++) {
		if (!(bh = bread(0x300 + drive * 5, 0))) {
			printk("Unable to read partition table of drive %d\n\r", drive);
			panic("");
		}
		if (bh->b_data[510] != 0x55 || (unsigned char)
		    bh->b_data[511] != 0xAA) {
			printk("Bad partition table on drive %d\n\r",drive);
			panic("");
		}
		p = 0x1BE + (void *)bh->b_data;
		for (i=1;i<5;i++,p++) {
			hd[i+5*drive].start_sect = p->start_sect;
			hd[i+5*drive].nr_sects = p->nr_sects;
		}
		brelse(bh);
	}

	for (i=0 ; i<5*MAX_HD ; i++)
		hd_sizes[i] = hd[i].nr_sects>>1 ;
	blk_size[MAJOR_NR] = hd_sizes;
	if (NR_HD)
		printk("Partition table%s ok.\n\r",(NR_HD>1)?"s":"");

	mount_root();

	return 100;
}
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

extern int sys_ls(int flags)
{
	struct m_inode * dir;
	int entries;
	int block, i, j;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;
	
	dir = current->pwd;
	entries = dir->i_size / (sizeof (struct dir_entry));
	i = 0;
	j = 0;
	
	block = dir->i_zone[0];
	if (!block)
	{
		panic("dir block 0 is 0. it's wrong.");
	}
	else;
	
	bh = bread(dir->i_dev, block);
	if (!bh)
	{
		panic("can not read the buffer head.");
	}
	else;
	
	de = (struct dir_entry *)bh->b_data;
	
	while (i < entries)
	{
		if (j >= DIR_ENTRIES_PER_BLOCK)
		{
			brelse(bh);
			bh = NULL;
			block = bmap(dir, i/DIR_ENTRIES_PER_BLOCK);
			if (!block)
			{
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			else;				
			
			bh = bread(dir->i_dev, block);
			if (!bh)
			{
				panic("can not read the buffer head.");
			}
			else;
			
			j = 0;			
			de = (struct dir_entry *)bh->b_data;
		}
		else;
		
		if (de->inode)
		{
			if (!flags)
			{
				printk(de->name);
				printk("\t");
			}
			else
			{
				printk("directary entry name: %s\t\t", de->name);
				printk("inode No: %d\n", de->inode);
			}
		}
		else;
			
		de++;
		i++;	
		j++;
	}
	
	if (!flags)
	{
		printk("\n");
	}
	else;
	
	brelse(bh);
	return 100;
}

extern int sys_pwd(void)
{
	struct m_inode *cur_dir, *parent_dir, *child_dir;
	int dev, block_nr;
	struct buffer_head *bh01, *bh02, *tmp_bh;
	void * buf_addr;
	char * dirname_buf;
	char ** dir_name;
	int dir_count;
	struct dir_entry *de01, *de02;
	int parent_i_num, cur_i_num, child_i_num;
	int i, j, argc, length;
	
	buf_addr = (void *)get_free_page();
	dirname_buf = (char *)buf_addr;
	dir_name = (char **)(dirname_buf + 2048);
	dirname_buf[1023] = 0;
	i = 1022;
	
	cur_dir = current->pwd;
	cur_i_num = cur_dir->i_num;
	if (cur_i_num == ROOT_INO)
	{
		dirname_buf[i] = '/';
		printk(dirname_buf + i);
		goto plane;
	}
	
	dev = cur_dir->i_dev;
	block_nr = cur_dir->i_zone[0];
	bh01 = bread(dev, block_nr);
	de01 = (struct dir_entry *)bh01->b_data;
	de01++;
	parent_i_num = de01->inode;
	brelse(bh01);
	parent_dir = iget(dev, parent_i_num);

	do
	{
		child_dir = cur_dir;
		child_i_num = child_dir->i_num;
		if (child_i_num != current->pwd->i_num)
		{
			iput(child_dir);
		}
		else;

		cur_dir = parent_dir;
		cur_i_num = cur_dir->i_num;
		dev = cur_dir->i_dev;
		block_nr = cur_dir->i_zone[0];
		bh01 = bread(dev, block_nr);
		de01 = (struct dir_entry *)bh01->b_data;
		de01++;
		parent_i_num = de01->inode;
		brelse(bh01);
		parent_dir = iget(dev, parent_i_num);
		
		bh02 = find_entry_by_inum(cur_dir, child_i_num, &de02);
		if (!bh02)
		{
			panic("can not read directary entry.");
		}
		else;
		
		length = 0;
		while (de02->name[length])
		{
			length++;
		}

#ifdef NO_TRUNCATE
	if (length > NAME_LEN)
		return NULL;
#else
	if (length > NAME_LEN)
		length = NAME_LEN;
#endif
		
		while (length > 0)
		{
			length--;
			dirname_buf[i] = de02->name[length];
			i--;
		}
		dirname_buf[i--] = '/';
		brelse(bh02);
	} while (cur_i_num != ROOT_INO || parent_i_num != ROOT_INO);
	
	iput(cur_dir);
	iput(cur_dir);
	/*****************************************************
		上面的代码释放的是根目录。注意，这仅仅是因为，在最后两次
	  do-while 循环里面，我们通过iget()函数多获取了两次i节点，增加
	  了两次根目录节点的引用计数。所以上句代码是将最后两次迭代所增加的
	  引用技术给减去，并非是说要彻底释放根节点。根节点，是在系统初始化
	  的时候，由 mount_root() 负责读取到内存中的。
		**************************************************/
	
	/*******************************************************
		普通的i节点都有上面目录来包含它的i节点号和目录项名字。唯有根节点
	  没有这样的上层目录。因为，根节点是至高无上的。对于根目录，我们需要
	  单独地来处理。遇到了，直接来保存'/'。
	  保存好了以后，就要打印路径名了。
	********************************************************/
	printk(dirname_buf + i + 1);
plane:	
	free_page(buf_addr);
	return 100;	
}

extern struct buffer_head * find_entry_by_inum(
  struct m_inode * cur_dir, int child_i_num,
  struct dir_entry ** res_dir)
{
	int entries;
	int block, i, j;
	struct buffer_head * bh;
	struct dir_entry * de;
	struct super_block * sb;

	i = 0;
	j = 0;

	entries = cur_dir->i_size / (sizeof (struct dir_entry));

	if (!(block = cur_dir->i_zone[0]))
		return NULL;
	if (!(bh = bread(cur_dir->i_dev, block)))
		return NULL;

	de = (struct dir_entry *) bh->b_data;
	while (i < entries) {
		if (j >= DIR_ENTRIES_PER_BLOCK) {
			brelse(bh);
			bh = NULL;
			if (!(block = bmap(cur_dir, i / DIR_ENTRIES_PER_BLOCK)) ||
			    !(bh = bread(cur_dir->i_dev,block))) {
				i += DIR_ENTRIES_PER_BLOCK;
				continue;
			}
			else;
			
			j = 0;
			de = (struct dir_entry *) bh->b_data;
		}
		else;
		
		if (de->inode == child_i_num) {
			*res_dir = de;
			return bh;
		}
		else;

		de++;
		i++;
		j++;
	}
	
	brelse(bh);
	return NULL;
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
	
	block_nr = 1 + FIRST_PART_BLOCKNO;
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
	block_nr = 2 + psuper_block->s_imap_blocks + psuper_block->s_zmap_blocks + FIRST_PART_BLOCKNO;
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
	pinode->i_time = CURRENT_TIME;
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
	block_nr = root_dir_contents_blocknr + FIRST_PART_BLOCKNO;
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
	int start_head, start_cyl, start_sect;
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
	
	sector_num_lba = FIRST_PART_SECTNO;
	disk_buffer = (unsigned char *)DISK_BUFFER;
	part = (struct partition *)(disk_buffer + 0x1BE);
	memset(disk_buffer, 0, 1024);	
	
	heads = hd_info[index].head;
	cyls = hd_info[index].cyl;
	sectors = hd_info[index].sect;
	sector_total = heads * cyls * sectors;
	block_total = sector_total >> 1;
	block_total -= 32;
	
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
	
	start_sect = (FIRST_PART_SECTNO % sectors) + 1;
	tmp = FIRST_PART_SECTNO / sectors;
	start_head = tmp % heads;
	start_cyl = tmp / heads;
	
	part->boot_ind = 0x00;
	part->head = start_head;
	part->sector = (start_sect & 0x3F) | ((start_cyl >> 2) & 0xC0);
	part->cyl = start_cyl & 0xFF;
	part->sys_ind = 0x80;   //minix---0x80
	part->end_head = end_head;
	part->end_sector = (end_sect & 0x3F) | ((end_cyl >> 2) & 0xC0);
	part->end_cyl = end_cyl & 0xFF;
	part->start_sect = FIRST_PART_SECTNO;
	part->nr_sects = sector_total - FIRST_PART_SECTNO;
	disk_buffer[510] = 0x55;
	disk_buffer[511] = 0xAA;
	
	block_total -= FIRST_PART_BLOCKNO;
	
	if (!index)
	{
		write_first_disk(sector_num_lba, disk_buffer);
		write_first_disk(0, disk_buffer);
	}
	else
	{
		write_second_disk(sector_num_lba, disk_buffer);
		write_second_disk(0, disk_buffer);
	}
	sector_num_lba += 2;

// super block formating
	memset(disk_buffer, 0, 1024);
	super_block = disk_buffer;

	tmp = block_total/2;	//磁盘块除以2为i节点总数。
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








