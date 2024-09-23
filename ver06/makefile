CC = gcc
AS = as
LD = ld
CPP = cpp
INC = -I include/

# -fno-pic -fno-stack-protector -gstabs

CPPFLAGS = -nostdinc
DEBUG_FLAGS = -gstabs
CFLAGS = -Wall -m32 -fno-builtin -fno-stack-protector -W -Wstrict-prototypes \
 -Wmissing-prototypes $(DEBUG_FLAGS) $(INC) -c
BOOT_ASFLAGS = --32
ASFLAGS = $(BOOT_ASFLAGS) $(DEBUG_FLAGS)
LDFLAGS = -m elf_i386

BOOT_LDFLAGS = -Ttext 0x0 --oformat binary
SYS_LDFLAGS = -Ttext 0x10000 -e main -Map System.map

OBJS_INIT = Link/main.o

OBJS_LIB = Link/string.o Link/io.o Link/segment.o Link/system.o \
  Link/write_simple.o Link/print_sys_time.o Link/print_sys_info.o \
  Link/ctype.o Link/print_hd_info.o Link/format_hd.o

OBJS_KERNEL = Link/village.o Link/vsprintf.o Link/printk.o Link/panic.o Link/sched.o \
  Link/asm.o Link/traps.o Link/sys_call.o Link/mktime.o Link/sys.o \
  Link/printf.o Link/signal.o Link/fork.o Link/exit.o Link/shell.o

OBJS_BLK = Link/ll_rw_blk.o Link/ramdisk.o Link/hd.o

OBJS_CHR = Link/console.o Link/tty_io.o Link/keyboard.o Link/serial.o Link/rs_io.o \
  Link/pty.o Link/tty_ioctl.o

OBJS_FS = Link/bitmap.o Link/block_dev.o Link/buffer.o Link/char_dev.o Link/exec.o \
  Link/fcntl.o Link/file_dev.o Link/file_table.o Link/inode.o Link/ioctl.o \
  Link/namei.o Link/open.o Link/pipe.o Link/read_write.o Link/select.o \
  Link/stat.o Link/super.o Link/truncate.o

  
OBJS_MM = Link/memory.o Link/page.o


OBJS = $(OBJS_INIT) $(OBJS_LIB) $(OBJS_KERNEL) $(OBJS_BLK) \
  $(OBJS_CHR) $(OBJS_FS) $(OBJS_MM)
  

build: boot system

boot: boot.bin setup.bin head.bin

system: $(OBJS)
	$(LD) $(LDFLAGS) $(SYS_LDFLAGS) $^ -o $@



#########################################################
##################   boot chapter  ######################

boot.bin: Link/boot.o
	$(LD) $(LDFLAGS) $(BOOT_LDFLAGS) $< -o $@

setup.bin:Link/setup.o
	$(LD) $(LDFLAGS) $(BOOT_LDFLAGS) $< -o $@

head.bin: Link/head.o
	$(LD) $(LDFLAGS) $(BOOT_LDFLAGS) -Map Head.map $< -o $@

Link/boot.o:boot/boot.S
	$(AS) $(BOOT_ASFLAGS) $< -o $@

Link/setup.o:boot/setup.S
	$(AS) $(BOOT_ASFLAGS) $< -o $@
	
Link/head.o: boot/head.S
	$(AS) $(BOOT_ASFLAGS) $< -o $@

############   boot chapter end  ########################
#########################################################



#########################################################
#################    init chapter    ####################

Link/main.o: init/main.c include/stdio.h include/linux/head.h \
  include/asm/system.h include/linux/mm.h
	$(CC) $(CFLAGS) $< -o $@

################   init chapter end   ###################
#########################################################


#########################################################
#################   kernel chapter    ###################

Link/village.o: kernel/village.c include/linux/head.h include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

Link/vsprintf.o: kernel/vsprintf.c include/stdio.h include/string.h
	$(CC) $(CFLAGS) $< -o $@

Link/printk.o: kernel/printk.c include/stdio.h include/sys/sys.h
	$(CC) $(CFLAGS) $< -o $@

Link/panic.o: kernel/panic.c include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

Link/sched.o: kernel/sched.c include/stdio.h include/linux/sched.h include/asm/system.h \
  include/asm/io.h include/asm/segment.h include/sys/param.h include/sys/sys.h
	$(CC) $(CFLAGS) $< -o $@

Link/asm.o: kernel/asm.s
	$(AS) $(ASFLAGS) $< -o $@

Link/traps.o: kernel/traps.c include/stdio.h include/string.h \
  include/linux/head.h include/linux/sched.h include/asm/system.h \
  include/asm/segment.h include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/sys_call.o: kernel/sys_call.s
	$(AS) $(ASFLAGS) $< -o $@

Link/mktime.o: kernel/mktime.c include/stdio.h include/asm/io.h include/time.h \
  include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

Link/sys.o: kernel/sys.c include/stdio.h include/sys/sys.h
	$(CC) $(CFLAGS) $< -o $@

Link/printf.o: kernel/printf.c include/stdio.h include/sys_call.h
	$(CC) $(CFLAGS) $< -o $@

Link/signal.o: kernel/signal.c include/linux/sched.h include/asm/segment.h \
  include/errno.h
	$(CC) $(CFLAGS) $< -o $@

Link/fork.o: kernel/fork.c   include/stdio.h include/errno.h include/linux/sched.h \
  include/asm/segment.h include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/exit.o: kernel/exit.c include/stdio.h include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

Link/shell.o: kernel/shell.c include/stdio.h include/sys_call.h \
  include/linux/tty.h include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

############3##  kernel chapter end     #################
#########################################################


#########################################################
#################   lib chapter     #####################

Link/string.o: lib/string.c include/string.h
	$(CC) $(CFLAGS) $< -o $@

Link/io.o: lib/io.c include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/segment.o: lib/segment.c include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/system.o: lib/system.c include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/write_simple.o: lib/write_simple.c include/sys_call.h
	$(CC) $(CFLAGS) $< -o $@

Link/print_sys_time.o: lib/print_sys_time.c include/sys_call.h
	$(CC) $(CFLAGS) $< -o $@

Link/print_sys_info.o: lib/print_sys_info.c include/sys_call.h
	$(CC) $(CFLAGS) $< -o $@

Link/ctype.o: lib/ctype.c include/ctype.h
	$(CC) $(CFLAGS) $< -o $@

Link/print_hd_info.o: lib/print_hd_info.c include/sys_call.h
	$(CC) $(CFLAGS) $< -o $@

Link/format_hd.o: lib/format_hd.c include/sys_call.h
	$(CC) $(CFLAGS) $< -o $@

################  lib chapter end   #####################
#########################################################


#########################################################
#############    block  dev   chapter    ################

Link/ll_rw_blk.o: kernel/blk_drv/ll_rw_blk.c include/stdio.h include/errno.h \
  include/linux/sched.h include/asm/system.h include/linux/blk.h
	$(CC) $(CFLAGS) $< -o $@

Link/ramdisk.o: kernel/blk_drv/ramdisk.c include/stdio.h include/linux/sched.h \
  include/linux/blk.h include/linux/mm.h
	$(CC) $(CFLAGS) $< -o $@

Link/hd.o: kernel/blk_drv/hd.c include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

##############   block dev chapter end   ################
#########################################################

#########################################################
#############    char  dev   chapter    #################

Link/console.o: kernel/chr_drv/console.c include/asm/system.h include/asm/io.h \
  include/stdio.h include/linux/tty.h
	$(CC) $(CFLAGS) $< -o $@

Link/tty_io.o: kernel/chr_drv/tty_io.c include/stdio.h include/errno.h \
  include/linux/sched.h include/linux/tty.h
	$(CC) $(CFLAGS) $< -o $@

Link/keyboard.o: kernel/chr_drv/keyboard.S
	$(CC) $(CFLAGS) $< -o $@

Link/serial.o: kernel/chr_drv/serial.c include/linux/tty.h include/linux/sched.h \
  include/asm/system.h include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/rs_io.o: kernel/chr_drv/rs_io.s
	$(AS) $(ASFLAGS) $< -o $@

Link/pty.o: kernel/chr_drv/pty.c include/linux/tty.h include/linux/sched.h \
  include/asm/system.h include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/tty_ioctl.o: kernel/chr_drv/tty_ioctl.c include/errno.h include/linux/sched.h \
  include/stdio.h include/linux/tty.h include/asm/io.h include/asm/segment.h \
  include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

############    char dev end    ########################
########################################################


########################################################
#############  file chapter    #########################

Link/bitmap.o: fs/bitmap.c include/stdio.h include/string.h include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

Link/block_dev.o: fs/block_dev.c include/stdio.h include/errno.h include/linux/sched.h \
  include/asm/segment.h include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/buffer.o: fs/buffer.c include/stdio.h include/linux/sched.h include/asm/system.h \
  include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/char_dev.o: fs/char_dev.c include/stdio.h include/errno.h include/linux/sched.h \
  include/asm/segment.h include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/exec.o: fs/exec.c include/stdio.h include/errno.h include/string.h \
  include/a.out.h include/linux/sched.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/fcntl.o: fs/fcntl.c include/stdio.h include/string.h include/errno.h \
  include/linux/sched.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/file_dev.o: fs/file_dev.c include/stdio.h include/errno.h include/linux/sched.h \
  include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/file_table.o: fs/file_table.c include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

Link/inode.o: fs/inode.c include/stdio.h include/linux/sched.h include/string.h \
  include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/ioctl.o: fs/ioctl.c include/string.h include/errno.h include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

Link/namei.o: fs/namei.c include/stdio.h include/linux/sched.h include/asm/segment.h \
  include/string.h include/errno.h
	$(CC) $(CFLAGS) $< -o $@

Link/open.o: fs/open.c include/stdio.h include/string.h include/errno.h \
  include/time.h include/linux/sched.h include/linux/tty.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/pipe.o: fs/pipe.c include/stdio.h include/signal.h include/errno.h \
  include/linux/tty.h include/linux/sched.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/read_write.o: fs/read_write.c include/stdio.h include/errno.h include/linux/sched.h \
  include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/select.o: fs/select.c include/stdio.h include/linux/tty.h include/linux/sched.h \
  include/asm/segment.h include/asm/system.h include/string.h include/errno.h \
  include/time.h
	$(CC) $(CFLAGS) $< -o $@

Link/stat.o: fs/stat.c include/stdio.h include/errno.h include/linux/sched.h \
  include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/super.o: fs/super.c include/stdio.h include/linux/sched.h include/asm/system.h \
  include/errno.h
	$(CC) $(CFLAGS) $< -o $@

Link/truncate.o: fs/truncate.c include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

##############   file chapter end   ####################
########################################################


########################################################
################   memory chapter   ####################

Link/memory.o: mm/memory.c include/stdio.h include/linux/mm.h include/asm/system.h \
  include/linux/head.h include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

Link/page.o: mm/page.s
	$(AS) $(ASFLAGS) $< -o $@

###############    memory chapter end   ################
########################################################




	
clean:
	rm -f Link/* boot.bin setup.bin head.bin system

wdisk: wboot wsys

wboot: wmbr wset whead

wmbr:
	dd if=boot.bin of=Lab/fda.img bs=512 seek=0 count=1 conv=notrunc

wset:
	dd if=setup.bin of=Lab/fda.img bs=512 seek=1 count=4 conv=notrunc

whead:
	dd if=head.bin of=Lab/fda.img bs=512 seek=5 count=120 conv=notrunc

wsys:
	dd if=system of=Lab/fda.img bs=512 seek=125 count=2000 conv=notrunc








