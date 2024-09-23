#		本版本代码，同以前的代码一样，大量的代码，都会是抄写林纳斯
#	托瓦茨的。林纳斯的一些个版本声明，我就不添加到这里了。
#		本版本代码，依然属于是一种学习型的代码。它会大量地采用linux
#	0.12内核的代码，采用里面的诸多的机制。因为，到目前为止，我最为熟
#	悉的，是0.12内核。市面上，有许多的学习型内核，真象内核，田宇的64
#	位内核，以及川合秀实先生的内核，于渊先生的内核，这样的内核是有很多
#	的。而我最为熟悉，也最为喜欢的，还是林纳斯先生的代码。
#		我目前的学习程度，距离完全理解0.12内核，还有一定的距离。有时
#	会在阅读源代码时，理解一点0.12的机制。有时，是在调试代码时，理解了
#	一点0.12内核的知识。
#		Linux 内核，是一种精妙的内核，它应该可以算作是一种艺术了。
#		应该向林纳斯大神致敬。学习他的0.12内核时，常常会感叹Linux的
#	的精妙。好好向林纳斯大神学习吧。
#

.globl _start
SETUPLEN = 4				# nr of setup-sectors
BOOTSEG  = 0x07c0			# original address of boot-sector
INITSEG  = 0x9000			# we move boot here - out of the way
SETUPSEG = 0x9020			# setup starts here
SYSSEG   = 0x1000			# system loaded at 0x10000 (65536).
ROOT_DEV = 0x0306
SWAP_DEV = 0
ROOT_DEV = 0x0301

FIRST_PART_M =			2
FIRST_PART_BLOCKNO =	FIRST_PART_M * 1024
FIRST_PART_SECTNO =		FIRST_PART_M * 2048

.code16
.section .text
_start:
	movw	$BOOTSEG, %ax
	movw	%ax, %ds
	movw	$INITSEG, %ax
	movw	%ax, %es
	movw	$256, %cx
	subw	%si, %si
	subw	%di, %di
	rep
	movsw
	ljmp	$INITSEG, $1f

1:
	movw	%cs, %ax		
	movw	$0xff00, %dx	# stack pointer   0x9ff00

	movw	%ax, %ds
	movw	%ax, %es

	movw	%ax, %ss		# put stack at 0x9ff00 - 12.
	movw	%dx, %sp


# 清屏
#利用0x06号功能，上卷全部行，则可清屏。
# -----------------------------------------------------------
#INT 0x10   功能号:0x06	   功能描述:上卷窗口
#------------------------------------------------------
#输入：
#AH 功能号= 0x06
#AL = 上卷的行数(如果为0,表示全部)
#BH = 上卷行属性
#(CL,CH) = 窗口左上角的(X,Y)位置
#(DL,DH) = 窗口右下角的(X,Y)位置
#无返回值：

	movw     $0x0600, %ax 
	movw     $0x0700, %bx 
	movw     $0, %cx                    # 左上角: (0, 0)
	movw     $0x184f, %dx 		   # 右下角: (80,25),
				   # 因为VGA文本模式中，一行只能容纳80个字符,共25行。
				   # 下标从0开始，所以0x18=24,0x4f=79
	int     $0x10                     # int 10h


load_setup:
	movw $0x80, %dx
	movw $2, %cx
	movw $0x0200, %bx
	movb $4, %al
	movb $2, %ah
	int $0x13
	jnc display_msg1
	
1:
	movw $0x0703, %dx
	movw $msg3 - msg2, %cx
	movw $0x07, %bx
	movw $msg2, %bp
	movw $0x1301, %ax
	int $0x10
	
2:
	jmp 2b

display_msg1:
	movw $0x0106, %dx
	movw $msg2 - msg1, %cx
	movw $0x07, %bx
	movw $msg1, %bp
	movw $0x1301, %ax
	int $0x10

/******************************************************
	接下来的代码，其功能为加载head.s的代码。本版本代码里面，我对
  head.s的规划如下。
  1个页目录表：4k
  16个页表：64k
  任务0的用户栈：4k
  任务0的内核栈：4k
  GDT与IDT：4k
  DMA缓冲区：32K
  
	以上一共为112k。head.s本身也会有一些代码。这些个代码，应该
  不会超过4k，我取个4k整数，那么，112 + 4 = 116
  也就是，我会令head.s，固定地占用着116k的空间。
	然后，内存的116k到0x8FFFF的区域，可以供内核分配大一些的数据
  结构。比如i节点表，比如文件表。
*******************************************************/

load_head:
	cld
	movl $116 * 2, %ecx    # 148k, 296 sector amount
	movl $0x1000, %eax		# ax, es, buffer segment addr
	movl $5, %esi			# LBA sector num

read_it:
	movw %ax, %es
	xorw %di, %di
	call read_disk
	incl %esi
	addw $0x20, %ax
	loop read_it

	ljmp $0x9020, $0

read_disk:
	push %ax
	push %cx
	push %dx
	pushl %esi
	
	movw $0x1f2, %dx
	movb $1, %al
	outb %al, %dx
	
	movw $0x1f3, %dx
	movw %si, %ax
	outb %al, %dx
	
	movw $0x1f4, %dx
	movb %ah, %al
	outb %al, %dx
	
	movw $0x1f5, %dx
	movw $16, %cx
	shrl %cl, %esi
	movw %si, %ax
	outb %al, %dx
	
	movw $0x1f6, %dx
	movb $0xe0, %al
	orb %ah, %al
	outb %al, %dx
	
	movw $0x1f7, %dx
	movb $0x20, %al
	outb %al, %dx
	
.wait:
	inb %dx, %al
	andb $0x88, %al
	cmpb $0x08, %al
	jnz .wait

	movw $256, %cx
	movw $0x1f0, %dx

	cld
	
.readw:
	inw %dx, %ax
	stosw
	loop .readw
	
	popl %esi
	pop %dx
	pop %cx
	pop %ax	
	ret

msg1:
	.ascii "load system..."
	.byte 13, 10
msg2:
	.ascii "load system fail"
	.byte 13, 10
	.ascii "   please modify the load code..."
	.byte 13, 10
msg3:

.org 502
part_table:
	.long FIRST_PART_SECTNO
swap_dev:
	.word SWAP_DEV
root_dev:
	.word ROOT_DEV
boot_flag:
	.word 0xAA55












