.code64

.section	.text
.global	sse_check

sse_check:
    push %rbx 
    push %rcx
    push %rdx 

    mov $0x1, %rax        # Request CPU feature information
    cpuid

    test $(1 << 25), %rdx # Check if the SSE bit is set in rdx
    mov $0x1, %rax        # Assume SSE is available (rax = 1)

    jnz .good             # If the SSE bit is set, jump to .good

    xor %rax, %rax        # Otherwise, set rax to 0 (no SSE support)
.good:
    pop %rdx              # Restore the original value of edx
    pop %rcx              # Restore the original value of ecx
    pop %rbx              # Restore the original value of ebx
    ret


.global		sse_enable

sse_enable:
    # enable SSE
    mov %cr0, %rax
    and $~0x04, %ax
    or $0x2, %ax
    mov %rax, %cr0

    mov %cr4, %rax
    or $(3 << 9), %ax
    mov %rax, %cr4

    ret
