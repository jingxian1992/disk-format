
KERNEL_ELF_ADDR = 0x700000
KERNEL_LBA_SECT = 237
READ_SECT_AMOUNT = 0x1000
PT_NULL = 0
main = 0x103000

.globl _start, idt, gdt, pg_dir, print_str, put_char
.section .text
.code32

_start:
pg_dir:
	jmp startup_32

.org 0x1000
pg0:

.org 0x2000
pg1:

.org 0x3000
pg2:

.org 0x4000
pg3:

.org 0x5000
pg4:

.org 0x6000
pg5:

.org 0x7000
pg6:

.org 0x8000
pg7:

.org 0x9000
pg8:

.org 0xA000
pg9:

.org 0xB000
pga:

.org 0xC000
pgb:

.org 0xD000
pgc:

.org 0xE000
pgd:

.org 0xF000
pge:

.org 0x10000
pgf:

.org 0x11000
stack_info:
	.long stack_top
	.word 0x10

.org 0x12000
stack_top:

.org 0x13000    # stack top used for task 0 kernel state stack
				# and IDT start address
idt:
	.fill 256, 8, 0
	
gdt:
	.word 0, 0, 0, 0    # null descriptor
	
	.word 0x3FFF		#  code segment, 64M, start 0
	.word 0				#  readable, nonconforming
	.word 0x9A00
	.word 0x00C0
	
	.word 0x3FFF		#  data segment, 64M, start 0
	.word 0				#  read/write, upward-extend
	.word 0x9200
	.word 0x00C0
	
	.word 0, 0, 0, 0    #  temporary - not use
	.fill 252, 8, 0

dma_buffer:		# here, the addr is 0x14000
	.fill 0x8000, 1, 0

startup_32:		# here, the addr is 0x1C000
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	lss stack_info, %esp

	call setup_idt
	call setup_gdt
	
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	lss stack_info, %esp

	ljmp $8, $setup_paging

setup_paging:
	movl $1024 * 17, %ecx
	xorl %eax, %eax
	xorl %edi, %edi
	cld
	rep stosl
	
	movl $pg0+7, pg_dir+0
	movl $pg1+7, pg_dir+4
	movl $pg2+7, pg_dir+8
	movl $pg3+7, pg_dir+12
	movl $pg4+7, pg_dir+16
	movl $pg5+7, pg_dir+20
	movl $pg6+7, pg_dir+24
	movl $pg7+7, pg_dir+28
	movl $pg8+7, pg_dir+32
	movl $pg9+7, pg_dir+36
	movl $pga+7, pg_dir+40
	movl $pgb+7, pg_dir+44
	movl $pgc+7, pg_dir+48
	movl $pgd+7, pg_dir+52
	movl $pge+7, pg_dir+56
	movl $pgf+7, pg_dir+60

	movl $pg0, %edi
	movl $7, %eax
	cld
1:
	stosl
	addl $0x1000, %eax
	cmpl $0x4000000, %eax
	jl 1b
	
	xorl %eax, %eax
	movl %eax, %cr3
	
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0
	
	ljmp $8, $kernel_load

kernel_load:
	movl $KERNEL_ELF_ADDR, %ebx
	movl $KERNEL_LBA_SECT, %eax
	movl $READ_SECT_AMOUNT - 24, %ecx

read_it:
	call read_disk
	incl %eax
	addl $512, %ebx
	loop read_it

kernel_init:
	xorl %eax, %eax
	xorl %ebx, %ebx
	xorl %ecx, %ecx
	xorl %edx, %edx
	
	movw KERNEL_ELF_ADDR + 42, %dx    # the size of each program header
	movl KERNEL_ELF_ADDR + 28, %ebx    # offset against file of first program header 
	addl $KERNEL_ELF_ADDR, %ebx   # the addr of first program header
	movw KERNEL_ELF_ADDR + 44, %cx  # the count of program headers
	
.each_segment:
	cmpb $PT_NULL, 0(%ebx)
	je .PTNULL
	
	pushl 16(%ebx)
	movl 4(%ebx), %eax
	addl $KERNEL_ELF_ADDR, %eax
	pushl %eax
	pushl 8(%ebx)
	call mem_cpy
	addl $12, %esp
.PTNULL:
	addl %edx, %ebx
	loop .each_segment

prepare_to_kernel:	
	pushl $0
	pushl $0
	pushl $0
	pushl $village
	pushl $main
	ret

.align 4	
village:
	jmp village

.align 4
setup_idt:
	lea ignore_int, %edx
	movl $0x00080000, %eax
	movw %dx, %ax
	movw $0x8E00, %dx

	lea idt, %edi
	movl $256, %ecx
rp_sidt:
	movl %eax, (%edi)
	movl %edx, 4(%edi)
	addl $8, %edi
	loop rp_sidt
	lidt idt_desc
	ret

.align 4	
setup_gdt:
	lgdt gdt_desc
	ret

.align 4
int_msg:
	.asciz "Unknown interrupt\n"
.align 4
ignore_int:
	pushl %eax
	pushl %ecx
	pushl %edx
	push %ds
	push %es
	push %fs
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	pushl $int_msg
	call print_str
	addl $4, %esp
	pop %fs
	pop %es
	pop %ds
	popl %edx
	popl %ecx
	popl %eax
	iret

.align 4
mem_cpy:
	pushl %ebp
	movl %esp, %ebp
	pushl %ecx
	cld
	movl 8(%ebp), %edi
	movl 12(%ebp), %esi
	movl 16(%ebp), %ecx
	rep movsb

	popl %ecx
	movl %ebp, %esp
	popl %ebp
	ret

.align 4
print_str:
	pushl %ebp
	movl %esp, %ebp
	pushl %eax
	pushl %ecx
	movl 8(%ebp), %eax

.char_task:
	movb (%eax), %cl
	cmpb $0, %cl
	je .str_over
	pushl %ecx
	call put_char
	addl $4, %esp
	incl %eax
	jmp .char_task

.str_over:
	popl %ecx
	popl %eax
	movl %ebp, %esp
	popl %ebp
	ret

.align 4
put_char:
	pushl %ebp
	movl %esp, %ebp
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	push %ds
	push %es
	
	movl $0x10, %eax
	movw %ax, %ds
	movw %ax, %es
	
	xorl %eax, %eax
	xorl %ebx, %ebx
	xorl %ecx, %ecx
	xorl %edx, %edx
	
	movw $0x3D4, %dx
	movb $14, %al
	outb %al, %dx
	
	movw $0x3D5, %dx
	inb %dx, %al
	movb %al, %bh
	
	movw $0x3D4, %dx
	movb $15, %al
	outb %al, %dx
	
	movw $0x3D5, %dx
	inb %dx, %al
	movb %al, %bl
	
	movl 8(%ebp), %ecx
	cmpb $0xD, %cl
	je .CR
	cmpb $0xA, %cl
	je .LF
	cmpb $0x9, %cl
	je .HT
	cmpb $127, %cl
	je .BS
	cmpb $0x8, %cl
	je .BS
	
ordi_char:
	shll $1, %ebx
	movb %cl, 0xB8000(%ebx)
	movb $0x7, 0xB8001(%ebx)
	shrl $1, %ebx
	incl %ebx
	cmpw $2000, %bx
	jl .set_cursor
	jmp .scroll_screen
	
.CR:
.LF:
	movw $80, %cx
	movw %bx, %ax
	divb %cl
	movb $8, %cl
	shrw %cl, %ax
	subw %ax, %bx
	addw $80, %bx
	cmpw $2000, %bx
	jl .set_cursor
	jmp .scroll_screen
	
.HT:
	movw %bx, %dx
	andw $7, %dx
	movw $8, %ax
	subw %dx, %ax
	addw %ax, %bx
	cmpw $2000, %bx
	jl .set_cursor
	jmp .scroll_screen

.BS:
	decw %bx
	shlw $1, %bx
	movw $0x0720, 0xB8000(%ebx)
	shrw $1, %bx
	jmp .set_cursor

.scroll_screen:
	cld
	movl $160 + 0xB8000, %esi
	movl $0 + 0xB8000, %edi
	movl $160*24/4, %ecx
	rep movsl
	movl $3840 + 0xB8000, %edi
	movl $80, %ecx
	movw $0x0720, %ax
	rep stosw 
	movl $1920, %ebx

.set_cursor:
	movw $0x3d4, %dx
	movb $14, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	movb %bh, %al
	outb %al, %dx
	
	movw $0x3d4, %dx
	movb $15, %al
	outb %al, %dx
	
	movw $0x3d5, %dx
	movb %bl, %al
	outb %al, %dx
	
put_char_over:
	pop %es
	pop %ds
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	movl %ebp, %esp
	popl %ebp
	ret

/********************************************
	下面的函数为读盘函数。且用寄存器来传递参数。
	参数说明如下：
	eax=LBA扇区号    ebx=将数据写入的内存地址
	本段程序，主要是从真象还原移植过来的。
	不同的是，真象还原每次读多个扇区
	本函数每次只是读一个扇区。
*********************************************/
read_disk:
	pushl %eax
	pushl %ebx
	pushl %ecx

	movl %eax, %esi	   # 备份eax
#读写硬盘:
#第1步：设置要读取的扇区数
	movw $0x1f2, %dx
	movb $1, %al
	outb %al, %dx		#读取的扇区数,1个扇区

	movl %esi, %eax		#恢复eax

#  第2步：将LBA地址存入0x1f3 ~ 0x1f6
#  LBA地址7~0位写入端口0x1f3
	movw $0x1f3, %dx                       
	outb %al, %dx                          

#  LBA地址15~8位写入端口0x1f4
	movb $8, %cl
	shrl %cl, %eax
	movw $0x1f4, %dx
	outb %al, %dx

#  LBA地址23~16位写入端口0x1f5
	shrl %cl, %eax
	movw $0x1f5, %dx
	outb %al, %dx

	shrl %cl, %eax
	andb $0x0f, %al	   #lba第24~27位
	orb $0xe0, %al	   # 设置7～4位为1110,表示lba模式，主盘
	movw $0x1f6, %dx
	outb %al, %dx

#第3步：向0x1f7端口写入读命令，0x20 
	movw $0x1f7, %dx
	movb $0x20, %al                        
	outb %al, %dx

/**********************************************
	至此,硬盘控制器便从指定的lba地址(eax)处,读出1个扇
  区,下面检查硬盘状态,不忙就能把这1个扇区的数据读出来
***********************************************/
#第4步：检测硬盘状态
  .not_ready:		   #测试0x1f7端口(status寄存器)的的BSY位
      #同一端口,写时表示写入命令字,读时表示读入硬盘状态
	jmp 1f
1:	jmp 1f
1:	inb %dx, %al
	andb $0x88, %al	   #第4位为1表示硬盘控制器已准备好数据传输,第7位为1表示硬盘忙
	cmpb $0x08, %al
	jnz .not_ready	   #若未准备好,继续等。

#第5步：从0x1f0端口读数据
	movl $256, %ecx 	   
	movw $0x1f0, %dx 

.go_on_read:
	inw %dx, %ax		
	movw %ax, (%ebx) 
	addl $2, %ebx 
	loop .go_on_read
	
	popl %ecx
	popl %ebx
	popl %eax
	ret

.align 4
	.word 0
idt_desc:
	.word 256*8-1
	.long idt
	
.align 4
	.word 0
gdt_desc:
	.word 256*8-1
	.long gdt




