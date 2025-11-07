// NocturneOS - kernel initialization code for x86 platforms.

.code32
.set ALIGN,    1 << 0      # Alignment of loaded modules by page boundaries
.set MEMINFO,  1 << 1      # Request to provide memory map
.set VBE_MODE, 1 << 2      # VBE mode flag. GRUB will set it for us and provide info about it.
.set INIT_MBOOT_HEADER_MAGIC,           0x1BADB002
.set INIT_MBOOT_HEADER_FLAGS,           ALIGN | MEMINFO | VBE_MODE
.set INIT_MBOOT_CHECKSUM,               0x00000000 - (INIT_MBOOT_HEADER_MAGIC + INIT_MBOOT_HEADER_FLAGS)

.set STACK_SIZE, 1024 * 32  # 128 KB

.extern kmain

.section .mboot, "a", @progbits

.int INIT_MBOOT_HEADER_MAGIC
.int INIT_MBOOT_HEADER_FLAGS
.int INIT_MBOOT_CHECKSUM
.long 0, 0, 0, 0, 0     # Unused
.long 0                 # 0 - graphical mode
.long 800, 600, 32      # Width, height, depth

.section .bss
    .align 16
    stack_bottom:
        .skip STACK_SIZE
    stack_top:

.section .text
.globl __pre_init
__pre_init:
        cli

        # Load stack
        mov $stack_top, %esp

        call sse_enable

        # Initialize FPU
        push %eax

        mov %cr4, %eax
        or $0x200, %eax
        mov %eax, %cr4
        
        pop %eax

        # init FPU
        fninit
        fldcw (conword)

        push	%esp
        push	%ebx

        xor %ebp, %ebp

        call	kmain

        hlt

conword:
        .word 0x37f

loop:
        jmp	loop
