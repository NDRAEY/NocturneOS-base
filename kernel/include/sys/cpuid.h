/* SPDX-License-Identifier: GPL-3.0-or-later */

//
// Created by ndraey on 03.10.23.
// Modified by: NotYourFox on 06.09.25
//

/* 
 * FIXME: CPUID leafs features are arch-dependent a priori. 
 * Make cpu_info-like structures and capability definitions for each arch separately to improve portability.
 * cpu_has(), boot_cpu_has() should probably be also used mostly in arch-dependent code.
 * One can not expect boot_cpu_has(some_x86_feature) on ARMv7 targets.
 */

#pragma once

#include "arch/x86/cpu_vendors.h"
#include <common.h>
#include <lib/bitops.h>

/* x86-relative. */
#define NR_CAPS 5

SAYORI_INLINE
void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
	__asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(leaf));
}

SAYORI_INLINE
void cpuid_count(uint32_t leaf, uint32_t count, uint32_t* eax, uint32_t* ebx,
                                 uint32_t* ecx, uint32_t* edx) {
	__asm__ volatile("cpuid"
	             : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
	             : "0"(leaf), "2"(count));
}

struct cpu_info {
    const struct cpu_vendor* vendor;
    
	size_t manufacturer_id;
	
	size_t model_id;
	size_t family_id;
	
	size_t type_id;
	size_t brand_id;
	
	size_t stepping_id;

	char* brand_string;
	char* model_string;
	char* type_string;

	size_t l1_i_size;
	size_t l1_d_size;
	size_t l2_size;
	size_t l3_size;

	int cpuid_max_leaf;
	uint32_t extended_cpuid_max_leaf;
	
	union {
		uint32_t cpu_capability[NR_CAPS];
		unsigned long cpu_capability_alignment;
	};
};

extern struct cpu_info boot_cpu_info;

#define cpu_has(info, bit)          test_bit((unsigned long *)((info)->cpu_capability), bit)
#define cpu_set_cap(info, bit)      set_bit((unsigned long *)((info)->cpu_capability), bit)
#define cpu_clear_cap(info, bit)    clear_bit((unsigned long *)((info)->cpu_capability), bit)

#define boot_cpu_has(bit)           cpu_has(&boot_cpu_info, bit)

void cpu_get_info(struct cpu_info* out);
