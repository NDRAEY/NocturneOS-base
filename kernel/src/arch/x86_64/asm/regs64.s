.global	read_cr0
read_cr0:
      mov	%cr0, %rax
      ret
      
.global	write_cr0
write_cr0:
      push	%rbp
      mov	%rsp, %rbp
      mov	16(%rbp), %rax
      mov	%rax, %cr0
      pop	%rbp

      ret
      
.global	read_cr3
read_cr3:
      mov	%cr3, %rax
      ret
      
.global	write_cr3
write_cr3:
      push	%rbp
      mov	%rsp, %rbp
      mov	8(%rbp), %rax
      mov	%rax, %cr3
      pop	%rbp
      
      ret
      
.global	read_cr2
read_cr2:
      mov	%cr2, %rax
      ret
