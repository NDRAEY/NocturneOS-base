.global	read_cr0
read_cr0:
      mov	%cr0, %eax
      ret
      
.global	write_cr0
write_cr0:
      push	%ebp
      mov	%esp, %ebp
      mov	8(%ebp), %eax
      mov	%eax, %cr0
      pop	%ebp

      ret
      
.global	read_cr3
read_cr3:
      mov	%cr3, %eax
      ret
      
.global	write_cr3
write_cr3:
      push	%ebp
      mov	%esp, %ebp
      mov	8(%ebp), %eax
      mov	%eax, %cr3
      pop	%ebp
      
      ret
      
.global	read_cr2
read_cr2:
      mov	%cr2, %eax
      ret


.global get_regs
get_regs:
    pushl   %ebp
    movl    %esp, %ebp
    
    pusha
    pushl   %ds

    movl    8(%ebp), %eax
    
    # ds
    movw    %ds, %cx
    movzwl  %cx, %ecx
    movl    %ecx, (%eax)        # regs->ds
    
    # edi
    movl    24(%esp), %ecx      # edi from pusha
    movl    %ecx, 4(%eax)       # regs->edi
    
    # esi
    movl    28(%esp), %ecx      # esi from pusha
    movl    %ecx, 8(%eax)       # regs->esi
    
    # ebp
    movl    32(%esp), %ecx      # ebp from pusha
    movl    %ecx, 12(%eax)      # regs->ebp
    
    # esp - need to calculate original esp
    movl    36(%esp), %ecx      # esp from pusha
    movl    %ecx, 16(%eax)      # regs->esp
    
    # ebx
    movl    40(%esp), %ecx      # ebx from pusha
    movl    %ecx, 20(%eax)      # regs->ebx
    
    # edx
    movl    44(%esp), %ecx      # edx from pusha
    movl    %ecx, 24(%eax)      # regs->edx
    
    # ecx
    movl    48(%esp), %ecx      # ecx from pusha
    movl    %ecx, 28(%eax)      # regs->ecx
    
    # eax
    movl    52(%esp), %ecx      # eax from pusha
    movl    %ecx, 32(%eax)      # regs->eax
    
    movl    $0, 36(%eax)        # regs->int_num
    movl    $0, 40(%eax)        # regs->err_code

    movl    $0, 44(%eax)        # regs->eip
    movl    $0, 48(%eax)        # regs->cs
    movl    $0, 52(%eax)        # regs->eflags
    movl    $0, 56(%eax)        # regs->useresp
    movl    $0, 60(%eax)        # regs->ss
    
    # Clean up and return
    popl    %ds
    popa
    
    popl    %ebp
    ret