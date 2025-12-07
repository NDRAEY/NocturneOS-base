#pragma once

#include <common.h>

typedef	struct {
    uint32_t	ds;
    uint32_t	edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t	int_num, err_code;
    uint32_t	eip, cs, eflags, useresp, ss;
} __attribute__((packed)) registers_t;

// Not precise, but at least valid
void get_regs(registers_t* regs);