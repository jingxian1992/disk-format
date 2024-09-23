
KERNEL_ELF_ADDR = 0x100000
PT_NULL = 0
main = 0x10000

.globl _start, idt, gdt, pg_dir
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
stack_info:
	.long stack_top
	.word 0x10

.org 0xA000
stack_top:

.org 0xB000
startup_32:
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
	movl $1024 * 9, %ecx
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
	
	movl $pg0, %edi
	movl $7, %eax
	cld
1:
	stosl
	addl $0x1000, %eax
	cmpl $0x2000000, %eax
	jl 1b
	
	xorl %eax, %eax
	movl %eax, %cr3
	
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0
	
	ljmp $8, $kernel_move
	
kernel_move:
	movl $1024 * 60, %esi
	movl $KERNEL_ELF_ADDR, %edi
	movl $0x90000, %eax
	movl $main, %edx
	subl %edx, %eax
	movb $2, %cl
	shrl %cl, %eax
	movl %eax, %ecx
	cld
	rep movsl	
	
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

.align 4
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

.org 0xC000
idt:
	.fill 256, 8, 0
	
gdt:
	.word 0, 0, 0, 0    # null descriptor
	
	.word 0x1FFF      #  code segment, 32M, start 0, readable, nonconforming
	.word 0
	.word 0x9A00
	.word 0x00C0
	
	.word 0x1FFF     # data segment, 32M, start 0, read/write, up-data
	.word 0
	.word 0x9200
	.word 0x00C0
	
	.word 0, 0, 0, 0    #  temporary - not use
	
	.fill 252, 8, 0







