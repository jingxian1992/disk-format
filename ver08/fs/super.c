/*
 *  linux/fs/super.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 * super.c contains code to handle the super-block tables.
 */

#include <stdio.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <errno.h>
#include <sys/stat.h>

int sync_dev(int dev);
void wait_for_keypress(void);

/* set_bit uses setb, as gas doesn't recognize setc */
#define set_bit(bitnr,addr) ({ \
register int __res __asm__("ax"); \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

struct super_block super_block[NR_SUPER];
/* this is initialized in init/main.c */
int ROOT_DEV = 0;

static void lock_super(struct super_block * sb)
{
	cli();
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));
	sb->s_lock = 1;
	sti();
}

static void free_super(struct super_block * sb)
{
	cli();
	sb->s_lock = 0;
	wake_up(&(sb->s_wait));
	sti();
}

static void wait_on_super(struct super_block * sb)
{
	cli();
	while (sb->s_lock)
		sleep_on(&(sb->s_wait));
	sti();
}

struct super_block * get_super(int dev)
{
	struct super_block * s;

	if (!dev)
		return NULL;
	s = 0+super_block;
	while (s < NR_SUPER+super_block)
		if (s->s_dev == dev) {
			wait_on_super(s);
			if (s->s_dev == dev)
				return s;
			s = 0+super_block;
		} else
			s++;
	return NULL;
}

void put_super(int dev)
{
	struct super_block * sb;
	int i;

	if (dev == ROOT_DEV) {
		printk("root diskette changed: prepare for armageddon\n\r");
		return;
	}
	if (!(sb = get_super(dev)))
		return;
	if (sb->s_imount) {
		printk("Mounted disk changed - tssk, tssk\n\r");
		return;
	}
	lock_super(sb);
	sb->s_dev = 0;
	for(i=0;i<I_MAP_SLOTS;i++)
		brelse(sb->s_imap[i]);
	for(i=0;i<Z_MAP_SLOTS;i++)
		brelse(sb->s_zmap[i]);
	free_super(sb);
	return;
}

static struct super_block * read_super(int dev)
{
	struct super_block * s;
	struct buffer_head * bh;
	int i,block;

	if (!dev)
		return NULL;
	if (s = get_super(dev))
		return s;
	for (s = 0+super_block ;; s++) {
		if (s >= NR_SUPER+super_block)
			return NULL;
		if (!s->s_dev)
			break;
	}
	s->s_dev = dev;
	s->s_isup = NULL;
	s->s_imount = NULL;
	s->s_time = 0;
	s->s_rd_only = 0;
	s->s_dirt = 0;
	lock_super(s);
	if (!(bh = bread(dev,1))) {
		s->s_dev=0;
		free_super(s);
		return NULL;
	}
	*((struct d_super_block *) s) =
		*((struct d_super_block *) bh->b_data);
	brelse(bh);
	if (s->s_magic != SUPER_MAGIC) {
		s->s_dev = 0;
		free_super(s);
		return NULL;
	}
	for (i=0;i<I_MAP_SLOTS;i++)
		s->s_imap[i] = NULL;
	for (i=0;i<Z_MAP_SLOTS;i++)
		s->s_zmap[i] = NULL;
	block=2;
	for (i=0 ; i < s->s_imap_blocks ; i++)
		if (s->s_imap[i]=bread(dev,block))
			block++;
		else
			break;
	for (i=0 ; i < s->s_zmap_blocks ; i++)
		if (s->s_zmap[i]=bread(dev,block))
			block++;
		else
			break;
	if (block != 2+s->s_imap_blocks+s->s_zmap_blocks) {
		for(i=0;i<I_MAP_SLOTS;i++)
			brelse(s->s_imap[i]);
		for(i=0;i<Z_MAP_SLOTS;i++)
			brelse(s->s_zmap[i]);
		s->s_dev=0;
		free_super(s);
		return NULL;
	}
	s->s_imap[0]->b_data[0] |= 1;
	s->s_zmap[0]->b_data[0] |= 1;
	free_super(s);
	return s;
}

int sys_umount(char * dev_name)
{
	struct m_inode * inode;
	struct super_block * sb;
	int dev;

	if (!(inode=namei(dev_name)))
		return -ENOENT;
	dev = inode->i_zone[0];
	if (!S_ISBLK(inode->i_mode)) {
		iput(inode);
		return -ENOTBLK;
	}
	iput(inode);
	if (dev==ROOT_DEV)
		return -EBUSY;
	if (!(sb=get_super(dev)) || !(sb->s_imount))
		return -ENOENT;
	if (!sb->s_imount->i_mount)
		printk("Mounted inode has i_mount=0\n");
	for (inode=inode_table+0 ; inode<inode_table+NR_INODE ; inode++)
		if (inode->i_dev==dev && inode->i_count)
				return -EBUSY;
	sb->s_imount->i_mount=0;
	iput(sb->s_imount);
	sb->s_imount = NULL;
	iput(sb->s_isup);
	sb->s_isup = NULL;
	put_super(dev);
	sync_dev(dev);
	return 0;
}

int sys_mount(char * dev_name, char * dir_name, int rw_flag)
{
	struct m_inode * dev_i, * dir_i;
	struct super_block * sb;
	int dev;

	if (!(dev_i=namei(dev_name)))
		return -ENOENT;
	dev = dev_i->i_zone[0];
	if (!S_ISBLK(dev_i->i_mode)) {
		iput(dev_i);
		return -EPERM;
	}
	iput(dev_i);
	if (!(dir_i=namei(dir_name)))
		return -ENOENT;
	if (dir_i->i_count != 1 || dir_i->i_num == ROOT_INO) {
		iput(dir_i);
		return -EBUSY;
	}
	if (!S_ISDIR(dir_i->i_mode)) {
		iput(dir_i);
		return -EPERM;
	}
	if (!(sb=read_super(dev))) {
		iput(dir_i);
		return -EBUSY;
	}
	if (sb->s_imount) {
		iput(dir_i);
		return -EBUSY;
	}
	if (dir_i->i_mount) {
		iput(dir_i);
		return -EPERM;
	}
	sb->s_imount=dir_i;
	dir_i->i_mount=1;
	dir_i->i_dirt=1;		/* NOTE! we don't iput(dir_i) */
	return 0;			/* we do that in umount */
}

extern void mount_root(void)
{
	int i, free, block_nr, block_count;
	struct super_block * p;
	struct m_inode * mi;
	struct d_inode * di;
	struct buffer_head * bh;
	char * buf;
	struct d_super_block * tmp;

	if (32 != sizeof (struct d_inode))
		panic("bad i-node size");

	/******************************************************
		原版代码是从软盘里面加载文件系统。我这里，固定地，从硬盘
	  的第一个分区里面加载文件系统。设备号：0x0301 or 0x0306
	*******************************************************/
	if (ROOT_DEV != 0x0301) {
		panic("root dev num error.\n");
	}

	/*******************************************************
		此处，用于设置内存超级块的部分字段，都是从read_super函数里面复制
	  的代码，应该是没有啥问题的。
	********************************************************/
	p = super_block;
	p->s_dev = ROOT_DEV;
	p->s_isup = NULL;
	p->s_imount = NULL;
	p->s_time = 0;
	p->s_rd_only = 0;
	p->s_dirt = 0;
	lock_super(p);

	/**********************************************************
		此处，用于读取超级块。超级块逻辑块号为1，而LBA号为2。在这里，需要将
	  LBA号设置为块号的2倍。因为，一个磁盘块，包含着两个扇区。
	***********************************************************/
	block_nr = 1;	
	bh = bread(ROOT_DEV, block_nr);

	if (!bh)
	{
		panic("Unable to mount root");
	}
	else;

	/****************************************************
		读取了磁盘的超级块之后，将磁盘超级块结构的信息，复制到内存超
	  级块结构里面。将磁盘结构复制到内存结构以后，就可以释放缓冲头了。
	******************************************************/
	tmp = (struct d_super_block *)bh->b_data;
	p->s_ninodes = tmp->s_ninodes;
	p->s_nzones = tmp->s_nzones;
	p->s_imap_blocks = tmp->s_imap_blocks;
	p->s_zmap_blocks = tmp->s_zmap_blocks;
	p->s_firstdatazone = tmp->s_firstdatazone;
	p->s_log_zone_size = tmp->s_log_zone_size;
	p->s_max_size = tmp->s_max_size;
	p->s_magic = tmp->s_magic;
	brelse(bh);
	
	if (p->s_magic != SUPER_MAGIC)
	{
		printk("we only support minix 1.0 file system now.\n");
		panic("file system type error.");
	}
	else;

	/***********************************************
		从2号块开始读起。2号块，正是i节点位图块的起始块号。
	*************************************************/
	block_nr = 2;
	
	for (i=0 ; i < p->s_imap_blocks ; i++)
	{
		bh = bread(ROOT_DEV, block_nr);
		
		if (!bh)
		{
			panic("Unable to mount root");
		}
		else;

		p->s_imap[i] = bh;
		block_nr++;
	}

	for (i=0 ; i < p->s_zmap_blocks ; i++)
	{
		bh = bread(ROOT_DEV, block_nr);

		if (!bh)
		{
			panic("Unable to mount root");
		}
		else;

		p->s_zmap[i] = bh;
		block_nr++;
	}

	p->s_imap[0]->b_data[0] |= 3;
	p->s_zmap[0]->b_data[0] |= 3;
	free_super(p);
	
	mi = inode_table;
	mi->i_count = 1;
	mi->i_dev = ROOT_DEV;
	mi->i_num = ROOT_INO;

	block_nr = 2 + p->s_imap_blocks + p->s_zmap_blocks;
	bh = bread(ROOT_DEV, block_nr);

	if (!bh)
	{
		panic("Unable to mount root");
	}
	else;

	buf = bh->b_data;
	di = (struct d_inode *)buf;
	mi->i_mode = di->i_mode;
	mi->i_uid = di->i_uid;
	mi->i_size = di->i_size;
	mi->i_mtime = di->i_time;
	mi->i_gid = di->i_gid;
	mi->i_nlinks = di->i_nlinks;
	
	for (i = 0; i < 9; i++)
	{
		mi->i_zone[i] = di->i_zone[i];
	}
	
	brelse(bh);
	
	mi->i_count += 3 ;	/* NOTE! it is logically used 4 times, not 1 */
	p->s_isup = p->s_imount = mi;
	current->pwd = mi;
	current->root = mi;

	block_nr++;
	block_count = p->s_firstdatazone - block_nr;
	
	for (i = 0; i < block_count; i++)
	{
		bh = bread(ROOT_DEV, block_nr++);
		if (!bh)
		{
			panic("Unable to mount root");
		}
		else;
		
		brelse(bh);
	}
	
	bh = bread(ROOT_DEV, block_nr);
	if (!bh)
	{
		panic("Unable to mount root");
	}
	else;
	brelse(bh);

	free=0;
	i=p->s_nzones;
	while (i > 0)
	{
		if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))
		{
			free++;
		}
		i--;
	}
	printk("\t%d/%d free blocks\n\r",free,p->s_nzones);
	free=0;
	i=p->s_ninodes;
	while (i > 0)
	{
		if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))
		{
			free++;
		}
		i--;
	}
	printk("\t%d/%d free inodes\n\n",free,p->s_ninodes);
}

extern void super_block_init(void)
{
	int i, j;
	
	for (i = 0; i < NR_SUPER; i++)
	{
		super_block[i].s_ninodes = 0;
		super_block[i].s_nzones = 0;
		super_block[i].s_imap_blocks = 0;
		super_block[i].s_zmap_blocks = 0;
		super_block[i].s_firstdatazone = 0;
		super_block[i].s_log_zone_size = 0;
		super_block[i].s_max_size = 0;
		super_block[i].s_magic = 0;
		
		for (j = 0; j < I_MAP_SLOTS; j++)
		{
			super_block[i].s_imap[j] = NULL;
		}

		for (j = 0; j < Z_MAP_SLOTS; j++)
		{
			super_block[i].s_zmap[j] = NULL;
		}
		
		super_block[i].s_dev = 0;
		super_block[i].s_isup = NULL;
		super_block[i].s_imount = NULL;
		super_block[i].s_time = 0;
		super_block[i].s_wait = NULL;
		super_block[i].s_lock = 0;
		super_block[i].s_rd_only = 0;
		super_block[i].s_dirt = 0;
	};
}
