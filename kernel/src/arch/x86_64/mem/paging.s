.code64
.section .text
.globl reload_cr3
reload_cr3:
    mov %cr3, %rax
	mov %rax, %cr3
    ret