#ifndef _BLK_H
#define _BLK_H

#define NR_BLK_DEV	7

/***************************************
	由于在QEMU和virtualbox里面，硬盘first盘，它在BIOS读取参数时，总是出错，
  所以我将first盘设置为硬编码格式。在本文件所在的目录里面，我单独编写一个名为
  【硬盘编码.txt】的文本文件，用以记录几种尺寸的硬盘的编码方式。
********************************************/
#ifndef HD_PARAM_TABLE
#define HD_PARAM_TABLE 0x90080

#define HD0_CYL				20
#define HD0_HEAD			16
#define HD0_SECTOR			63
#define HD0_WPCOM			65535
#define HD0_CTL				0xC8
#define HD0_LZONE			20

#endif

#ifndef MAX_HD
#define MAX_HD		2
#endif

#ifndef DISK_BUFFER
#define DISK_BUFFER 0x93000
#endif

/*
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 *
 * 32 seems to be a reasonable number: enough to get some benefit
 * from the elevator-mechanism, but not so much as to lock a lot of
 * buffers when they are in the queue. 64 seems to be too many (easily
 * long pauses in reading when heavy writing/syncing is going on)
 */
#define NR_REQUEST	32

/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */
struct request {
	int dev;		/* -1 if no request */
	int cmd;		/* READ or WRITE */
	int errors;
	unsigned int sector;
	unsigned int nr_sectors;
	char * buffer;
	struct task_struct * waiting;
	struct buffer_head * bh;
	struct request * next;
};

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 */
extern int IN_ORDER(struct request * s1, struct request * s2);
extern void hd_interrupt(void);
extern void rd_load(void);

struct blk_dev_struct {
	void (*request_fn)(void);
	struct request * current_request;
};

extern struct blk_dev_struct blk_dev[NR_BLK_DEV];
extern struct request request[NR_REQUEST];
extern struct task_struct * wait_for_request;
extern int * blk_size[NR_BLK_DEV];
extern void (* do_hd)(void);

#define CURRENT (blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV DEVICE_NR(CURRENT->dev)

/* Hd controller regs. Ref: IBM AT Bios-listing */
#define HD_DATA		0x1f0	/* _CTL when writing */
#define HD_ERROR	0x1f1	/* see err-bits */
#define HD_NSECTOR	0x1f2	/* nr of sectors to read/write */
#define HD_SECTOR	0x1f3	/* starting sector */
#define HD_LCYL		0x1f4	/* starting cylinder */
#define HD_HCYL		0x1f5	/* high byte of starting cyl */
#define HD_CURRENT	0x1f6	/* 101dhhhh , d=drive, hhhh=head */
#define HD_STATUS	0x1f7	/* see status-bits */
#define HD_PRECOMP HD_ERROR	/* same io address, read=error, write=precomp */
#define HD_COMMAND HD_STATUS	/* same io address, read=status, write=cmd */

#define HD_CMD		0x3f6

/* Bits of HD_STATUS */
#define ERR_STAT	0x01
#define INDEX_STAT	0x02
#define ECC_STAT	0x04	/* Corrected error */
#define DRQ_STAT	0x08
#define SEEK_STAT	0x10
#define WRERR_STAT	0x20
#define READY_STAT	0x40
#define BUSY_STAT	0x80

/* Values for HD_COMMAND */
#define WIN_RESTORE		0x10
#define WIN_READ		0x20
#define WIN_WRITE		0x30
#define WIN_VERIFY		0x40
#define WIN_FORMAT		0x50
#define WIN_INIT		0x60
#define WIN_SEEK 		0x70
#define WIN_DIAGNOSE		0x90
#define WIN_SPECIFY		0x91

/* Bits for HD_ERROR */
#define MARK_ERR	0x01	/* Bad address mark ? */
#define TRK0_ERR	0x02	/* couldn't find track 0 */
#define ABRT_ERR	0x04	/* ? */
#define ID_ERR		0x10	/* ? */
#define ECC_ERR		0x40	/* ? */
#define	BBD_ERR		0x80	/* ? */

struct hd_info_struct {
	unsigned short cyl;	//柱面数，偏移0x0，字型。
	unsigned char head;	//磁头数，偏移0x2，字节。
	unsigned short rcur_cyl;	//开始减小写电流的柱面，偏移0x3，字型。
	unsigned short wpcom;	//开始写前预补偿柱面号（乘4），偏移0x5，字型
	unsigned char ECC_len;	//最大ECC猝发长度，偏移0x7，字节
	unsigned char ctl;		//控制字节（驱动器步进选择），偏移0x8，字节
	unsigned char std_timeout;	//标准超时值，偏移0x9，字节
	unsigned char fmt_timeout;	//格式化超时值，偏移0xA，字节
	unsigned char check_timeout;	//检测驱动器超时值，偏移0xB，字节
	unsigned short lzone;	//磁头着陆（停止）柱面号，偏移0xC，字型
	unsigned char sect;		//每磁道扇区数，偏移0xE，字节
	unsigned char preserve_byte;		//保留，偏移0xF，字节
} __attribute__ ((packed));
extern struct hd_info_struct hd_info[MAX_HD];

struct partition {
	unsigned char boot_ind;		/* 0x80 - active (unused) */
	unsigned char head;		/* ? */
	unsigned char sector;		/* ? */
	unsigned char cyl;		/* ? */
	unsigned char sys_ind;		/* ? */
	unsigned char end_head;		/* ? */
	unsigned char end_sector;	/* ? */
	unsigned char end_cyl;		/* ? */
	unsigned int start_sect;	/* starting sector counting from 0 */
	unsigned int nr_sects;		/* nr of sectors in partition */
};

void read_first_disk_base(unsigned int sector_num_lba, unsigned short * buffer);
void read_second_disk_base(unsigned int sector_num_lba, unsigned short * buffer);
void write_first_disk_base(unsigned int sector_num_lba, unsigned short * buffer);
void write_second_disk_base(unsigned int sector_num_lba, unsigned short * buffer);
void read_first_disk(unsigned int sector_num_lba, unsigned short * buffer);
void read_second_disk(unsigned int sector_num_lba, unsigned short * buffer);
void write_first_disk(unsigned int sector_num_lba, unsigned short * buffer);
void write_second_disk(unsigned int sector_num_lba, unsigned short * buffer);

#endif
