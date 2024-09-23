CC = gcc
AS = as
LD = ld
CPP = cpp
INC = -I include/
MAIN_ADDR = 0x103000

# -fno-pic -fno-stack-protector -gstabs

CPPFLAGS = -nostdinc
DEBUG_FLAGS = -gstabs
CFLAGS = -Wall -m32 -fno-builtin -fno-stack-protector -W \
  -Wstrict-prototypes -Wmissing-prototypes $(DEBUG_FLAGS) \
  $(INC) -c
BOOT_ASFLAGS = --32
ASFLAGS = $(BOOT_ASFLAGS) $(DEBUG_FLAGS)
LDFLAGS = -m elf_i386

BOOT_LDFLAGS = -Ttext 0x0 --oformat binary
SYS_LDFLAGS = -Ttext $(MAIN_ADDR) -e main -Map System.map

OBJS_INIT = Link/main.o

OBJS_LIB =  Link/ctype.o  Link/errno.o \
  Link/io.o Link/malloc.o \
  Link/segment.o Link/string.o \
  Link/system.o Link/write.o

# Link/close.o Link/dup.o Link/execve.o Link/_exit.o
# Link/open01.o Link/setsid.o Link/wait.o 

OBJS_KERNEL = Link/vsprintf.o Link/printk.o Link/panic.o \
  Link/asm.o Link/traps.o Link/sched.o Link/exit.o \
  Link/mktime.o Link/fork.o Link/signal.o Link/sys.o \
  Link/sys_call.o Link/api01.o Link/shell.o

OBJS_BLK = Link/hd.o Link/ll_rw_blk.o

OBJS_CHR = Link/console.o Link/keyboard.o Link/tty_io.o Link/rs_io.o \
  Link/serial.o Link/tty_ioctl.o

OBJS_FS = Link/bitmap.o Link/block_dev.o Link/buffer.o \
  Link/char_dev.o Link/exec.o Link/fcntl.o Link/file_dev.o \
  Link/file_table.o Link/inode.o Link/ioctl.o Link/namei.o \
  Link/open.o Link/pipe.o Link/read_write.o Link/select.o \
  Link/stat.o Link/super.o Link/truncate.o
  
OBJS_MM = Link/memory.o Link/page.o Link/swap.o

OBJS = $(OBJS_INIT) $(OBJS_LIB) $(OBJS_KERNEL) $(OBJS_BLK) \
  $(OBJS_CHR) $(OBJS_FS) $(OBJS_MM)

compile: boot system

boot: bootsect.bin setup.bin head.bin

system: $(OBJS)
	$(LD) $(LDFLAGS) $(SYS_LDFLAGS) $^ -o $@

#########################################################
##################   boot chapter  ######################

bootsect.bin: Link/bootsect.o
	$(LD) $(LDFLAGS) $(BOOT_LDFLAGS) $< -o $@

setup.bin: Link/setup.o
	$(LD) $(LDFLAGS) $(BOOT_LDFLAGS) $< -o $@

head.bin: Link/head.o
	$(LD) $(LDFLAGS) $(BOOT_LDFLAGS) -Map Head.map $< -o $@

Link/bootsect.o: boot/bootsect.s
	$(AS) $(BOOT_ASFLAGS) $< -o $@

Link/setup.o:boot/setup.s
	$(AS) $(BOOT_ASFLAGS) $< -o $@
	
Link/head.o: boot/head.s
	$(AS) $(BOOT_ASFLAGS) $< -o $@

############   boot chapter end  ########################
#########################################################

#########################################################
############## init chapter #############################

Link/main.o: init/main.c include/stdio.h include/linux/sched.h \
  include/string.h include/asm/io.h include/asm/system.h \
  include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

#################  init chapter end  ####################
#########################################################

#########################################################
##################  lib chapter #########################

Link/close.o: lib/close.c include/unistd.h
	$(CC) $(CFLAGS) $< -o $@

Link/ctype.o: lib/ctype.c include/ctype.h
	$(CC) $(CFLAGS) $< -o $@

Link/dup.o: lib/dup.c include/unistd.h
	$(CC) $(CFLAGS) $< -o $@

Link/errno.o: lib/errno.c
	$(CC) $(CFLAGS) $< -o $@

Link/execve.o: lib/execve.c include/unistd.h
	$(CC) $(CFLAGS) $< -o $@

Link/_exit.o: lib/_exit.c include/unistd.h
	$(CC) $(CFLAGS) $< -o $@

Link/io.o: lib/io.c include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/malloc.o: lib/malloc.c include/stdio.h include/linux/mm.h \
  include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/open01.o: lib/open01.c include/unistd.h include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

Link/segment.o: lib/segment.c include/asm/system.h \
  include/linux/head.h
	$(CC) $(CFLAGS) $< -o $@

Link/setsid.o: lib/setsid.c include/unistd.h
	$(CC) $(CFLAGS) $< -o $@

Link/string.o: lib/string.c include/string.h
	$(CC) $(CFLAGS) $< -o $@

Link/system.o: lib/system.c include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/wait.o: lib/wait.c include/unistd.h include/sys/wait.h
	$(CC) $(CFLAGS) $< -o $@

Link/write.o: lib/write.c include/unistd.h
	$(CC) $(CFLAGS) $< -o $@

###############   lib chapter end   #####################
#########################################################


#########################################################
####################  kernel chapter  ###################

Link/vsprintf.o: kernel/vsprintf.c include/stdio.h \
  include/string.h
	$(CC) $(CFLAGS) $< -o $@

Link/printk.o: kernel/printk.c include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

Link/panic.o: kernel/panic.c include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

Link/asm.o: kernel/asm.s
	$(AS) $(ASFLAGS) $< -o $@

Link/traps.o: kernel/traps.c include/string.h include/linux/head.h \
  include/stdio.h include/asm/system.h include/asm/segment.h \
  include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/sched.o: kernel/sched.c include/stdio.h include/linux/sched.h \
  include/asm/system.h include/asm/io.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/exit.o: kernel/exit.c include/stdio.h include/errno.h \
  include/signal.h include/sys/wait.h include/linux/sched.h \
  include/linux/tty.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/mktime.o: kernel/mktime.c include/stdio.h \
  include/sys/time.h
	$(CC) $(CFLAGS) $< -o $@

Link/fork.o: kernel/fork.c include/stdio.h include/errno.h \
  include/linux/sched.h include/asm/segment.h \
  include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/signal.o: kernel/signal.c include/stdio.h \
  include/linux/sched.h include/asm/segment.h \
  include/signal.h include/errno.h
	$(CC) $(CFLAGS) $< -o $@

Link/sys.o: kernel/sys.c include/stdio.h \
  include/errno.h include/linux/sched.h include/linux/tty.h \
  include/linux/config.h include/asm/segment.h \
  include/sys/time.h include/sys/utsname.h \
  include/sys/resource.h include/string.h
	$(CC) $(CFLAGS) $< -o $@

Link/sys_call.o: kernel/sys_call.s
	$(AS) $(ASFLAGS) $< -o $@

Link/api01.o: kernel/api01.c include/stdio.h \
  include/linux/sys.h include/linux/tty.h \
  include/sys/utsname.h
	$(CC) $(CFLAGS) $< -o $@

Link/shell.o: kernel/shell.c include/stdio.h \
  include/unistd.h include/linux/tty.h \
  include/linux/sched.h include/asm/segment.h \
  include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

#############  kernel chapter end  ######################
#########################################################

#########################################################
#############   block device chapter   ##################

Link/hd.o: kernel/blk_drv/hd.c include/stdio.h \
  include/linux/sched.h include/asm/system.h \
  include/asm/io.h include/asm/segment.h include/sys/time.h \
  include/linux/blk.h
	$(CC) $(CFLAGS) $< -o $@

Link/ll_rw_blk.o: kernel/blk_drv/ll_rw_blk.c include/stdio.h \
  include/errno.h include/linux/sched.h include/asm/system.h \
  include/linux/blk.h
	$(CC) $(CFLAGS) $< -o $@

#############   block device chapter end   ##############
#########################################################

#########################################################
##############  char device chapter  ####################

Link/console.o: kernel/chr_drv/console.c include/stdio.h \
  include/asm/system.h include/asm/io.h include/linux/tty.h
	$(CC) $(CFLAGS) $< -o $@

Link/keyboard.o: kernel/chr_drv/keyboard.s
	$(AS) $(ASFLAGS) $< -o $@

Link/tty_io.o: kernel/chr_drv/tty_io.c include/ctype.h \
  include/errno.h include/signal.h include/string.h \
  include/linux/sched.h include/linux/tty.h include/asm/segment.h \
  include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/rs_io.o: kernel/chr_drv/rs_io.s
	$(AS) $(ASFLAGS) $< -o $@

Link/serial.o: kernel/chr_drv/serial.c include/linux/tty.h \
  include/linux/sched.h include/asm/system.h include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/tty_ioctl.o: kernel/chr_drv/tty_ioctl.c include/stdio.h \
  include/errno.h include/linux/tty.h include/linux/sched.h \
  include/linux/tty.h include/asm/io.h include/asm/segment.h \
  include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

#############   char device chapter end   ###############
#########################################################

###########################################
############   file chapter   #############

Link/bitmap.o: fs/bitmap.c include/string.h include/linux/sched.h \
  include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

Link/block_dev.o: fs/block_dev.c include/errno.h include/linux/sched.h \
  include/stdio.h include/asm/segment.h include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/buffer.o: fs/buffer.c include/linux/config.h include/linux/sched.h \
  include/stdio.h include/asm/system.h include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/char_dev.o: fs/char_dev.c include/errno.h include/sys/types.h include/linux/sched.h \
  include/stdio.h include/asm/segment.h include/asm/io.h
	$(CC) $(CFLAGS) $< -o $@

Link/exec.o: fs/exec.c include/signal.h include/errno.h include/string.h \
  include/sys/stat.h include/a.out.h include/linux/fs.h include/linux/sched.h \
  include/stdio.h include/linux/mm.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/fcntl.o: fs/fcntl.c include/string.h include/errno.h include/linux/sched.h \
  include/stdio.h include/asm/segment.h include/fcntl.h include/sys/stat.h
	$(CC) $(CFLAGS) $< -o $@

Link/file_dev.o: fs/file_dev.c include/errno.h include/fcntl.h include/linux/sched.h include/stdio.h \
  include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/file_table.o: fs/file_table.c include/linux/fs.h
	$(CC) $(CFLAGS) $< -o $@

Link/inode.o: fs/inode.c include/string.h include/sys/stat.h include/linux/sched.h \
  include/stdio.h include/linux/mm.h include/asm/system.h
	$(CC) $(CFLAGS) $< -o $@

Link/ioctl.o: fs/ioctl.c include/string.h include/errno.h include/sys/stat.h \
  include/linux/sched.h
	$(CC) $(CFLAGS) $< -o $@

Link/namei.o: fs/namei.c include/linux/sched.h include/stdio.h include/asm/segment.h \
  include/string.h include/fcntl.h include/errno.h include/const.h include/sys/stat.h
	$(CC) $(CFLAGS) $< -o $@

Link/open.o: fs/open.c include/string.h include/errno.h \
  include/fcntl.h include/sys/types.h include/sys/time.h \
  include/sys/stat.h include/linux/sched.h include/linux/tty.h \
  include/stdio.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/pipe.o: fs/pipe.c include/signal.h include/errno.h \
  include/linux/tty.h include/linux/sched.h include/linux/mm.h \
  include/asm/segment.h include/stdio.h
	$(CC) $(CFLAGS) $< -o $@

Link/read_write.o: fs/read_write.c include/sys/stat.h include/errno.h include/sys/types.h \
  include/stdio.h include/linux/sched.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/select.o: fs/select.c include/linux/fs.h include/stdio.h include/linux/tty.h \
  include/linux/sched.h include/asm/segment.h include/asm/system.h include/sys/stat.h \
  include/sys/types.h include/string.h include/const.h include/errno.h include/sys/time.h \
  include/signal.h
	$(CC) $(CFLAGS) $< -o $@

Link/stat.o: fs/stat.c include/errno.h include/sys/stat.h include/linux/fs.h \
  include/linux/sched.h include/stdio.h include/asm/segment.h
	$(CC) $(CFLAGS) $< -o $@

Link/super.o: fs/super.c include/linux/config.h include/linux/sched.h include/stdio.h \
  include/asm/system.h include/errno.h include/sys/stat.h
	$(CC) $(CFLAGS) $< -o $@

Link/truncate.o: fs/truncate.c include/linux/sched.h include/sys/stat.h
	$(CC) $(CFLAGS) $< -o $@

############   file chapter   #############
###########################################

#########################################################
##############  memory chapter   ########################

Link/memory.o: mm/memory.c include/stdio.h include/asm/system.h \
  include/linux/head.h
	$(CC) $(CFLAGS) $< -o $@

Link/page.o: mm/page.s
	$(AS) $(ASFLAGS) $< -o $@

Link/swap.o: mm/swap.c include/stdio.h \
  include/string.h include/linux/mm.h \
  include/linux/sched.h include/linux/head.h
	$(CC) $(CFLAGS) $< -o $@

###############  memory chapter end  ####################
#########################################################

clean:
	rm -f Link/* bootsect.bin setup.bin head.bin system *.map

wdisk: wboot wsys

wboot: wmbr wset whead

wmbr:
	dd if=bootsect.bin of=Lab/hda.img bs=512 seek=0 count=1 conv=notrunc

wset:
	dd if=setup.bin of=Lab/hda.img bs=512 seek=1 count=4 conv=notrunc

whead:
	dd if=head.bin of=Lab/hda.img bs=512 seek=5 count=232 conv=notrunc

wsys:
	dd if=system of=Lab/hda.img bs=512 seek=237 count=12000 conv=notrunc





