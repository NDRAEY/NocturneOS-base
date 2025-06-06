#pragma once

#include	"common.h"
#include	"sys/isr.h"

#define		SYSCALL					0x80

typedef size_t syscall_fn_t (size_t, size_t, size_t);
// typedef size_t syscall_fn_t (void*, void*, void*);

#define		NUM_CALLS	128

void init_syscalls(void);
extern size_t syscall_entry_call(void* entry_point, void* param1, void* param2, void* param3);
void syscall_handler(registers_t regs);
