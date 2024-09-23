#
# SYS_SIZE is the number of clicks (16 bytes) to be loaded.
# 0x3000 is 0x30000 bytes = 196kB, more than enough for current
# versions of linux
#
SYSSIZE = 0x8000
#
#	bootsect.s		(C) 1991 Linus Torvalds
#
# bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves
# iself out of the way to address 0x90000, and jumps there.
#
# It then loads 'setup' directly after itself (0x90200), and the system
# at 0x10000, using BIOS interrupts. 
#
# NOTE# currently system is at most 8*65536 bytes long. This should be no
# problem, even in the future. I want to keep it simple. This 512 kB
# kernel size should be enough, especially as this doesn't contain the
# buffer cache as in minix
#
# The loader has been made as simple as possible, and continuos
# read errors will result in a unbreakable loop. Reboot by hand. It
# loads pretty fast by getting whole sectors at a time whenever possible.

.globl _start

SETUPLEN = 4				# nr of setup-sectors
BOOTSEG  = 0x07c0			# original address of boot-sector
INITSEG  = 0x9000			# we move boot here - out of the way
SETUPSEG = 0x9020			# setup starts here
SYSSEG   = 0x1000			# system loaded at 0x10000 (65536).
ENDSEG   = SYSSEG + SYSSIZE		# where to stop loading

# ROOT_DEV:	0x000 - same type of floppy as boot.
#		0x301 - first partition on first drive etc
ROOT_DEV = 0x0101   #   I use the ramdisk as the root file device.

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
	ljmp	$INITSEG, $go
	
go:	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es
# put stack at 0x9ff00.
	movw	%ax, %ss
	movw	$0xFF00, %sp		# arbitrary value >>512

# load the setup-sectors directly after the bootblock.
# Note that 'es' is already set up.

load_setup:
	movw	$0x0000, %dx		# drive 0, head 0
	movw	$0x0002, %cx		# sector 2, track 0
	movw	$0x0200, %bx		# address = 512, in INITSEG
	movw	$0x0200+SETUPLEN, %ax	# service 2, nr of sectors
	int	$0x13			# read it
	jnc	ok_load_setup		# ok - continue
	movw	$0x0000, %dx
	movw	$0x0000, %ax		# reset the diskette
	int	$0x13
	jmp	load_setup

ok_load_setup:

# Get disk drive parameters, specifically nr of sectors/track

	movb	$0x00, %dl
	movw	$0x0800, %ax		# AH=8 is get drive parameters
	int	$0x13
	movb	$0x00, %ch
	movw	%cx, %cs:sectors
	movw	$INITSEG, %ax
	movw	%ax, %es

# Print some inane message

#	movb	$0x03, %ah		# read cursor pos
#	xorb	%bh, %bh
#	int	$0x10

	movw    $0x600,%ax		#  本段代码，用于清屏。清理bochs的屏幕输出。
	movw    $0x700,%bx
	movw    $0,%cx          
	movw    $0x184f, %dx	  
	int     $0x10

	movw	$0x0109, %dx
	
	movw	$24, %cx
	movw	$0x0007, %bx		# page 0, attribute 7 (normal)
	movw	$msg1, %bp
	movw	$0x1301, %ax		# write string, move cursor
	int	$0x10

# ok, we've written the message, now
# we want to load the system (at 0x10000)

	movw	$SYSSEG, %ax
	movw	%ax, %es		# segment of 0x010000
	call	read_it
	call	kill_motor

# After that we check which root-device to use. If the device is
# defined (#= 0), nothing is done and the given device is used.
# Otherwise, either /dev/PS0 (2,28) or /dev/at0 (2,8), depending
# on the number of sectors that the BIOS reports currently.

	movw	%cs:root_dev, %ax
	cmpw	$0, %ax
	jne	root_defined
	movw	%cs:sectors, %bx
	movw	$0x0208, %ax		# /dev/ps0 - 1.2Mb
	cmpw	$15, %bx
	je	root_defined
	movw	$0x021c, %ax		# /dev/PS0 - 1.44Mb
	cmpw	$18, %bx
	je	root_defined
undef_root:
	jmp undef_root
root_defined:
	movw	%ax, %cs:root_dev

# after that (everyting loaded), we jump to
# the setup-routine loaded directly after
# the bootblock:
	
	ljmp	$SETUPSEG, $0

# This routine loads the system at address 0x10000, making sure
# no 64kB boundaries are crossed. We try to load it as fast as
# possible, loading whole tracks whenever we can.
#
# in:	es - starting address segment (normally 0x1000)
#
sread:	.word 1+SETUPLEN	# sectors read of current track
head:	.word 0			# current head
track:	.word 0			# current track

read_it:
	movw %es, %ax
	testw $0x0fff, %ax
die:	jne die			# es must be at 64kB boundary
	xorw %bx, %bx		# bx is starting address within segment
	
rp_read:
	movw %es, %ax
	cmpw $ENDSEG, %ax		# have we loaded all yet?
	jb ok1_read
	ret
ok1_read:
	movw %cs:sectors, %ax
	subw sread, %ax
	movw %ax, %cx
	shlw $9, %cx
	addw %bx, %cx
	jnc ok2_read
	je ok2_read
	xorw %ax, %ax
	subw %bx, %ax
	shrw $9, %ax
	
ok2_read:
	call read_track
	movw %ax, %cx
	addw sread, %ax
	cmpw %cs:sectors, %ax
	jne ok3_read
	movw $1, %ax
	subw head, %ax
	jne ok4_read
	incw track
	
ok4_read:
	movw %ax, head
	xorw %ax, %ax
ok3_read:
	movw %ax, sread
	shlw $9, %cx
	addw %cx, %bx
	jnc rp_read
	movw %es, %ax
	addw $0x1000, %ax
	movw %ax, %es
	xorw %bx, %bx
	jmp rp_read

read_track:
	pushw %ax
	pushw %bx
	pushw %cx
	pushw %dx
	movw track, %dx
	movw sread, %cx
	incw %cx
	movb %dl, %ch
	movw head, %dx
	movb %dl, %dh
	movb $0, %dl
	andw $0x0100, %dx
	movb $2, %ah
	int $0x13
	jc bad_rt
	popw %dx
	popw %cx
	popw %bx
	popw %ax
	ret
	
bad_rt:	movw $0, %ax
	movw $0, %dx
	int $0x13
	popw %dx
	popw %cx
	popw %bx
	popw %ax
	jmp read_track

/*
 * This procedure turns off the floppy drive motor, so
 * that we enter the kernel in a known state, and
 * don't have to worry about it later.
 */
kill_motor:
	pushw %dx
	movw $0x3f2, %dx
	movb $0, %al
	outb %al, %dx
	popw %dx
	ret

sectors:
	.word 0

msg1:
	.byte 13,10
	.ascii "Loading system ..."
	.byte 13,10,13,10

.org 508
root_dev:
	.word ROOT_DEV
boot_flag:
	.word 0xAA55


