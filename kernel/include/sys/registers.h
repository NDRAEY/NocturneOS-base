#pragma once

#include <common.h>

struct registers {
    uint32_t	ds;
    uint32_t	edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t	int_num, err_code;
    uint32_t	eip, cs, eflags, useresp, ss;
} __attribute__((packed));

typedef	struct	registers	registers_t;
