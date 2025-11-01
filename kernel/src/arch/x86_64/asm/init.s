.code32
.set ALIGN,    1 << 0      # Alignment of loaded modules by page boundaries
.set MEMINFO,  1 << 1      # Request to provide memory map
.set VBE_MODE, 1 << 2      # VBE mode flag. GRUB will set it for us and provide info about it.
.set INIT_MBOOT_HEADER_MAGIC,           0x1BADB002
.set INIT_MBOOT_HEADER_FLAGS,           ALIGN | MEMINFO | VBE_MODE
.set INIT_MBOOT_CHECKSUM,               0x00000000 - (INIT_MBOOT_HEADER_MAGIC + INIT_MBOOT_HEADER_FLAGS)

.set STACK_SIZE, 1024 * 16  # 16 KB

.section .mboot, "a", @progbits

.int INIT_MBOOT_HEADER_MAGIC
.int INIT_MBOOT_HEADER_FLAGS
.int INIT_MBOOT_CHECKSUM
.long 0, 0, 0, 0, 0     # Unused
.long 0                 # 0 - graphical mode
.long 800, 600, 32      # Width, height, depth

// The kernel stack
.section .bss
    .align 16
    stack_bottom:
        .skip STACK_SIZE
    stack_top:
        .skip 4

.section .bootstrap_tables, "aw", @progbits
    .align 4096
    pml4t:
        .skip 4096
    pdpt:
        .skip 4096
    pdt:
        .skip 4096
    pt:
        .skip 4096

.extern KERNEL_BASE_pos

.section .data
kernel_start: .int 0
kernel_end: .int 0
kernel_size: .int 0

// We are currently in 32-bit protected mode
.section .text
.globl __pre_init
__pre_init:
    cli
    // Load stack
    mov $stack_top, %esp
    xor %ebp, %ebp

    // Calculate kernel size (will be used in paging setup)
    push %ebx
    push %ecx

    mov $(KERNEL_BASE_pos), %ebx
    mov $(KERNEL_END_pos), %ecx

    sub %ebx, %ecx
    movl $(kernel_size), %ebx

    mov %ecx, (%ebx)

    pop %ecx
    pop %ebx

    // Setup paging hierarchy
    // PML4T
    mov $pml4t, %edi
    mov $pdpt, %esi
    or $(1 << 0 | 1 << 1), %esi
    mov %esi, (%edi)
    
    // PDPTE
    mov $pdpt, %edi
    mov $pdt, %esi
    or $(1 << 0 | 1 << 1), %esi
    mov %esi, (%edi)

    // Map first 2 MB
    mov $pdt, %edi
    mov $0, %esi
    or $(1 << 0 | 1 << 1 | 1 << 7), %esi
    mov %esi, (%edi)

    // Load PML4T

    mov $pml4t, %eax
    mov %eax, %cr3

    // Enable PAE
    mov %cr4, %eax
    or $(1 << 5 | 1 << 4), %eax
    mov %eax, %cr4

    mov $0xC0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    mov %cr0, %eax
    or $(1 << 0 | 1 << 31), %eax
    mov %eax, %cr0

    lgdt gdtr
    ljmp $0x08, $go64

.align 8
gdtr:
    .word gdt_end - gdt_base
    .quad gdt_base

gdt_base:
    .quad 0
    .word 0
    .word 0
    .byte 0
    .byte 0x9a
    .byte 0x20
    .byte 0
    .word 0xffff
    .word 0
    .byte 0
    .byte 0x92
    .byte 0
    .byte 0
gdt_end:

.code64
go64:
    cli
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    finit

    callq arch_init

    cli
    hlt

    .lp: jmp .lp
