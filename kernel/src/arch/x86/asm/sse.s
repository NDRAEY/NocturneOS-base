.section	.text
.global	sse_check

sse_check:
    push %ebx 
    push %ecx
    push %edx 

    mov $0x1, %eax        # Request CPU feature information
    cpuid

    test $(1 << 25), %edx # Check if the SSE bit is set in edx
    mov $0x1, %eax        # Assume SSE is available (eax = 1)

    jnz .good             # If the SSE bit is set, jump to .good

    xor %eax, %eax        # Otherwise, set eax to 0 (no SSE support)
.good:
    pop %edx              # Restore the original value of edx
    pop %ecx              # Restore the original value of ecx
    pop %ebx              # Restore the original value of ebx
    ret


.global		sse_enable

sse_enable:
    # enable SSE
    mov %cr0, %eax
    and $~0x04, %ax
    or $0x2, %ax
    mov %eax, %cr0

    mov %cr4, %eax
    or $(3 << 9), %ax
    mov %eax, %cr4

    ret
