/*
 * This file has definitions for some important file table
 * structures etc.
 */

#ifndef _FS_H_
#define _FS_H_

/* devices are as follows: (same as minix, so we can use the minix
 * file system. These are major numbers.)
 *
 * 0 - unused (nodev)
 * 1 - /dev/mem
 * 2 - /dev/fd
 * 3 - /dev/hd
 * 4 - /dev/ttyx
 * 5 - /dev/tty
 * 6 - /dev/lp
 * 7 - unnamed pipes
 */

extern int IS_SEEKABLE(int x);

#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

void buffer_init(int buffer_end);
extern int MAJOR(int a);
extern int MINOR(int a);

#define NAME_LEN 14
#define ROOT_INO 1

#define I_MAP_SLOTS 8
#define Z_MAP_SLOTS 8
#define SUPER_MAGIC 0x137F

#define NR_OPEN 20
#define NR_INODE 64
#define NR_FILE 64
#define NR_SUPER 8
#define NR_HASH 307
#define BLOCK_SIZE 1024
#define BLOCK_SIZE_BITS 10

#ifndef MAX_LOG_ZONE_AMOUNT
#define MAX_LOG_ZONE_AMOUNT (8192 * 8)
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

int INODES_PER_BLOCK;   // ((BLOCK_SIZE)/(sizeof (struct d_inode)))
int DIR_ENTRIES_PER_BLOCK;   //  ((BLOCK_SIZE)/(sizeof (struct dir_entry)))

#define NIL_FILP	((struct file *)0)
#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

#define S_IFMT  00170000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define O_ACCMODE	00003
#define O_RDONLY	   00
#define O_WRONLY	   01
#define O_RDWR		   02
#define O_CREAT		00100	/* not fcntl */
#define O_EXCL		00200	/* not fcntl */
#define O_NOCTTY	00400	/* not fcntl */
#define O_TRUNC		01000	/* not fcntl */
#define O_APPEND	02000
#define O_NONBLOCK	04000	/* not fcntl */
#define O_NDELAY	O_NONBLOCK

#define BUFFER_END 0x200000

#define I_TYPE          0170000
#define I_DIRECTORY	0040000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000
#define I_SET_UID_BIT   0004000
#define I_SET_GID_BIT   0002000

/* Defines for fcntl-commands. Note that currently
 * locking isn't supported, and other things aren't really
 * tested.
 */
#define F_DUPFD		0	/* dup */
#define F_GETFD		1	/* get f_flags */
#define F_SETFD		2	/* set f_flags */
#define F_GETFL		3	/* more flags (cloexec) */
#define F_SETFL		4
#define F_GETLK		5	/* not implemented */
#define F_SETLK		6
#define F_SETLKW	7

/* for F_[GET|SET]FL */
#define FD_CLOEXEC	1	/* actually anything with low bit set goes */

/* Ok, these are locking features, and aren't implemented at any
 * level. POSIX wants them.
 */
#define F_RDLCK		0
#define F_WRLCK		1
#define F_UNLCK		2

typedef char buffer_block[BLOCK_SIZE];

struct stat {
	unsigned short	st_dev;
	unsigned short	st_ino;
	unsigned short	st_mode;
	unsigned char	st_nlink;
	unsigned short	st_uid;
	unsigned short	st_gid;
	unsigned short	st_rdev;
	int	st_size;
	unsigned int	st_atime;
	unsigned int	st_mtime;
	unsigned int	st_ctime;
};

struct buffer_head {
	char * b_data;			/* pointer to data block (1024 bytes) */
	unsigned int b_blocknr;	/* block number */
	unsigned short b_dev;		/* device (0 = free) */
	unsigned char b_uptodate;
	unsigned char b_dirt;		/* 0-clean,1-dirty */
	unsigned char b_count;		/* users using this block */
	unsigned char b_lock;		/* 0 - ok, 1 -locked */
	struct task_struct * b_wait;
	struct buffer_head * b_prev;
	struct buffer_head * b_next;
	struct buffer_head * b_prev_free;
	struct buffer_head * b_next_free;
};

struct d_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned int i_size;
	unsigned int i_time;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
};

struct m_inode {
	unsigned short i_mode;
	unsigned short i_uid;
	unsigned int i_size;
	unsigned int i_mtime;
	unsigned char i_gid;
	unsigned char i_nlinks;
	unsigned short i_zone[9];
/* these are in memory also */
	struct task_struct * i_wait;
	struct task_struct * i_wait2;	/* for pipes */
	unsigned int i_atime;
	unsigned int i_ctime;
	unsigned short i_dev;
	unsigned short i_num;
	unsigned short i_count;
	unsigned char i_lock;
	unsigned char i_dirt;
	unsigned char i_pipe;
	unsigned char i_mount;
	unsigned char i_seek;
	unsigned char i_update;
};

struct file {
	unsigned short f_mode;
	unsigned short f_flags;
	unsigned short f_count;
	struct m_inode * f_inode;
	int f_pos;
};

struct super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned int s_max_size;
	unsigned short s_magic;
/* These are only in memory */
	struct buffer_head * s_imap[8];
	struct buffer_head * s_zmap[8];
	unsigned short s_dev;
	struct m_inode * s_isup;
	struct m_inode * s_imount;
	unsigned int s_time;
	struct task_struct * s_wait;
	unsigned char s_lock;
	unsigned char s_rd_only;
	unsigned char s_dirt;
};

struct d_super_block {
	unsigned short s_ninodes;
	unsigned short s_nzones;
	unsigned short s_imap_blocks;
	unsigned short s_zmap_blocks;
	unsigned short s_firstdatazone;
	unsigned short s_log_zone_size;
	unsigned int s_max_size;
	unsigned short s_magic;
};

struct dir_entry {
	unsigned short inode;
	char name[NAME_LEN];
};

/* Once again - not implemented, but ... */
struct flock {
	short l_type;
	short l_whence;
	int l_start;
	int l_len;
	int l_pid;
};

struct ustat {
	int f_tfree;
	unsigned short f_tinode;
	char f_fname[6];
	char f_fpack[6];
};

extern struct m_inode inode_table[NR_INODE];
extern struct file file_table[NR_FILE];
extern struct super_block super_block[NR_SUPER];
extern struct buffer_head * start_buffer;
extern int NR_BUFFERS;
extern int ROOT_DEV;
extern char acc_mode_str[5];

extern struct task_struct * PIPE_READ_WAIT(struct m_inode * inode);
extern struct task_struct * PIPE_WRITE_WAIT(inode);
extern unsigned short PIPE_HEAD(struct m_inode * inode);
extern unsigned short PIPE_TAIL(struct m_inode * inode);
extern int PIPE_SIZE(struct m_inode * inode);
extern int PIPE_EMPTY(struct m_inode * inode);
extern int PIPE_FULL(struct m_inode * inode);
extern int S_ISLNK(unsigned short m);
extern int S_ISREG(unsigned short m);
extern int S_ISDIR(unsigned short m);
extern int S_ISCHR(unsigned short m);
extern int S_ISBLK(unsigned short m);
extern int S_ISFIFO(unsigned short m);

extern int chmod(const char *_path, unsigned short mode);
extern int fstat(int fildes, struct stat *stat_buf);
extern int mkdir(const char *_path, unsigned short mode);
extern int mkfifo(const char *_path, unsigned short mode);
extern int stat(const char *filename, struct stat *stat_buf);
extern unsigned short umask(unsigned short mask);

extern void truncate(struct m_inode * inode);
extern void sync_inodes(void);
extern int bmap(struct m_inode * inode,int block);
extern int create_block(struct m_inode * inode,int block);
extern struct m_inode * namei(const char * pathname);
extern struct m_inode * lnamei(const char * pathname);
extern int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode);
extern void iput(struct m_inode * inode);
extern struct m_inode * iget(int dev,int nr);
extern struct m_inode * get_empty_inode(void);
extern struct m_inode * get_pipe_inode(void);
extern struct buffer_head * get_hash_table(int dev, int block);
extern struct buffer_head * getblk(int dev, int block);
extern void ll_rw_block(int rw, struct buffer_head * bh);
extern void ll_rw_page(int rw, int dev, int nr, char * buffer);
extern void brelse(struct buffer_head * buf);
extern struct buffer_head * bread(int dev,int block);
extern void bread_page(unsigned int addr,int dev,int b[4]);
extern struct buffer_head * breada(int dev,int block,...);
extern int new_block(int dev);
extern int free_block(int dev, int block);
extern struct m_inode * new_inode(int dev);
extern void free_inode(struct m_inode * inode);
extern int sync_dev(int dev);
extern struct super_block * get_super(int dev);

extern void mount_root(void);

#endif
