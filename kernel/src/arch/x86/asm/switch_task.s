.extern		current_thread
.extern		current_proc
.extern		tss

.global		task_switch_v2
task_switch_v2:
    pushf
    
    cli

    push %ebx
    push %esi
    push %edi
    push %ebp

    mov 24(%esp), %eax  # Current task
    mov 28(%esp), %ebx  # Next task

    cmp %eax, %ebx
    je .no_need

    # Save current thread's stack
    mov	%esp, 28(%eax)

    # Set current_thread to next entry
    mov %ebx, current_thread

    # Load thread's stack
    mov %ebx, %edx
    mov 28(%edx), %esp

    # Load stack_top to tss
    mov 40(%edx), %eax
    mov $tss, %edx
    mov %eax, 4(%edx)

    # Load process' page directory
    # Load our process structure
    mov	current_thread, %ebx
    mov 12(%ebx), %eax
    mov %eax, current_proc

    mov current_proc, %ebx

    # Load our page directory address
    mov 12(%ebx), %ebx

    # Reload TLB
    mov %cr3, %eax
    cmp %eax, %ebx

    # If Page Directories are equal, we don't have to load it.
    je .no_need

    mov %ebx, %cr3

    .no_need:

    pop %ebp
    pop %edi
    pop %esi
    pop %ebx

    popf

    ret
