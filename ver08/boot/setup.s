
.globl _start
.section .text
.code16

_start:
	cld

# Get hd0 data
	movw	$0x0000, %ax
	movw	%ax, %ds
	ldsw	4*0x41, %si
	movw	$0x9000, %ax
	movw	%ax, %es
	movw	$0x0080, %di
	movw	$0x10, %cx
	rep
	movsb

# Get hd1 data
	movw	$0x0000, %ax
	movw	%ax, %ds
	ldsw	4*0x46, %si
	movw	$0x9000, %ax
	movw	%ax, %es
	movw	$0x0090, %di
	movw	$0x10, %cx
	rep
	movsb

# Check that there IS a hd1 :-)
	movw	$0x01500, %ax
	movb	$0x81, %dl
	int	$0x13
	jc	no_disk1
	cmpb	$3, %ah
	je	is_disk1

no_disk1:
	movw	$0x9000, %ax
	movw	%ax, %es
	movw	$0x0090, %di
	movw	$0x10, %cx
	movw	$0x00, %ax
	rep
	stosb

is_disk1:
###########  now, we start to come to protect mode.
###########   first,  we need to disable all interrupt.	
	cli
	movw $0, %ax
	cld
do_move:
	movw %ax, %es
	addw $0x1000, %ax
	cmpw $0x3000, %ax
	jz load_gdt_idt
	movw %ax, %ds
	xorw %si, %si
	xorw %di, %di
	movw $0x8000, %cx
	rep movsw
	jmp do_move

load_gdt_idt:
	movw $0x9020, %ax
	movw %ax, %ds
	lgdt gdt_desc
	lidt idt_desc

Open_A20:
	inb $0x92, %al
	orb $2, %al
	outb %al, $0x92

init_8259A:
	movb $0x11, %al
	outb %al, $0x20
	jmp 1f
1:	jmp 1f
1:	outb %al, $0xA0
	jmp 1f
1:	jmp 1f
1:	movb $0x20, %al
	outb %al, $0x21
	jmp 1f
1:	jmp 1f
1:	movb $0x28, %al
	outb %al, $0xA1
	jmp 1f
1:	jmp 1f
1:	movb $0x04, %al
	outb %al, $0x21
	jmp 1f
1:	jmp 1f
1:	movb $0x02, %al
	outb %al, $0xA1
	jmp 1f
1:	jmp 1f
1:	movb $0x01, %al
	outb %al, $0x21
	jmp 1f
1:	jmp 1f
1:	outb %al, $0xA1
	jmp 1f
1:	jmp 1f
1:	movb $0xFF, %al
	outb %al, $0x21
	jmp 1f
1:	jmp 1f
1:	outb %al, $0xA1
	

##### open PE bit in cr0
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0

flush_line:
	ljmp $8, $0


.align 8
gdt:
	.word 0, 0, 0, 0
	
	.word 0x3FFF
	.word 0
	.word 0x9A00
	.word 0x00C0
	
	.word 0x3FFF
	.word 0
	.word 0x9200
	.word 0x00C0

	.word 0
idt_desc:
	.word 0, 0, 0

	.word 0
gdt_desc:
	.word 23
	.word gdt+0x200, 0x9













