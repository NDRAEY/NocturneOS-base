#pragma once

#include	"common.h"

#ifdef NOCTURNE_X86
#include	"arch/x86/isr.h"
#endif

#define		SYSCALL					0x80

#define SYSCALL_YIELD 21

typedef size_t syscall_fn_t (size_t, size_t, size_t, size_t, size_t);

void init_syscalls(void);
void syscall_handler(registers_t* regs);
