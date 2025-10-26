/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Created by: NotYourFox, 2025 */

#include "arch/x86/cpu_vendors.h"
#include <mem/vmm.h>
#include <stdint.h>
#include <sys/cpuid.h>
#include <lib/string.h>
#include <arch/x86/cpufeature.h>
#include <io/ports.h>

struct cpu_info boot_cpu_info;

unsigned int x86_family(unsigned int vfms) {
	unsigned int res;

	res = (vfms >> 8) & 0xF;

	if (res == 0xf) res += (vfms >> 20) & 0xFF;

	return res;
}

unsigned int x86_model(unsigned int vfms) {
	unsigned int family, model;

	family = x86_family(vfms);

	model = (vfms >> 4) & 0xF;

	if (family >= 0x6) model += ((vfms >> 16) & 0xF) << 4;

	return model;
}

unsigned int x86_stepping(unsigned int vfms) {
	return vfms & 0xF;
}

static inline bool cpuid_present( ) {
	uint32_t eax, ebx;

	__asm__ volatile("pushf\n\t"
	             "movl\t(%%esp), %0\n\t"
	             "movl\t%0, %1\n\t"
	             "xorl\t$0x00200000, %0\n\t"
	             "pushl\t%0\n\t"
	             "popf\n\t"
	             "pushf\n\t"
	             "popl\t%0\n\t"
	             "popf\n\t"

	             : "=&r"(eax), "=&r"(ebx));

	// rax should have the bit flipped if CPUID is supported
	return (eax & 0x200000) != (ebx & 0x200000);
}

void cpu_get_info(struct cpu_info* c) {
    memset(c, 0x00, sizeof(struct cpu_info));
    
    uint32_t eax, ebx, ecx, edx;
    
    if (!cpuid_present())  {
        c->cpuid_max_leaf = -1;
        return;
    }
    
    c->brand_string = kcalloc(sizeof(char), 13);
    cpuid(0x00000000, (unsigned int *)&c->cpuid_max_leaf, (unsigned int *)&c->brand_string[0],
	      (unsigned int *)&c->brand_string[8], (unsigned int *)&c->brand_string[4]);
    
    /* Supports CPUID, so at least 4h (Intel 486). */
	c->family_id = 4;

	c->vendor = cpu_vendor_by_signature(c->brand_string);

	if(c->vendor) {
		c->manufacturer_id = c->vendor->vendor;
	}
	
	if (c->cpuid_max_leaf >= 0x00000001) {
		cpuid(0x00000001, &eax, &ebx, &ecx, &edx);
		c->family_id = x86_family(eax);
		c->model_id = x86_model(eax);
		c->stepping_id = x86_stepping(eax);

		c->cpu_capability[CPUID_1_EDX] = edx;
		c->cpu_capability[CPUID_1_ECX] = ecx;
	}
	
	if (c->cpuid_max_leaf >= 0x00000006) {
	    cpuid(0x00000006, &eax, &ebx, &ecx, &edx);
		c->cpu_capability[CPUID_6_EAX] = eax;
	}
	
	if (c->cpuid_max_leaf >= 0x00000007) {
		cpuid_count(0x00000007, 0, &eax, &ebx, &ecx, &edx);
		c->cpu_capability[CPUID_7_EBX] = ebx;
	}
	
	c->model_string = kcalloc(sizeof(char), 49);
	cpuid(0x80000000, &c->extended_cpuid_max_leaf, &ebx, &ecx, &edx);
	if (c->extended_cpuid_max_leaf >= 0x80000004) {
		cpuid(0x80000002, (uint32_t *)&c->model_string[0], (uint32_t *)&c->model_string[4],
		      (uint32_t *)&c->model_string[8], (uint32_t *)&c->model_string[12]);
		cpuid(0x80000003, (uint32_t *)&c->model_string[16],
		      (uint32_t *)&c->model_string[20], (uint32_t *)&c->model_string[24],
		      (uint32_t *)&c->model_string[28]);
		cpuid(0x80000004, (uint32_t *)&c->model_string[32],
		      (uint32_t *)&c->model_string[36], (uint32_t *)&c->model_string[40],
		      (uint32_t *)&c->model_string[44]);
	} else {
		char* legacy_model = cpu_vendor_legacy_model(c->vendor, c->family_id, c->model_id);
		strcpy(c->model_string, legacy_model);
	}

	if (c->extended_cpuid_max_leaf >= 0x80000007) {
		cpuid(0x80000007, &eax, &ebx, &ecx, &edx);
		c->cpu_capability[CPUID_80000007_EDX] = edx;
	}

	/* QEMU/KVM have invariant TSC, but do not advertise it. */
	if (!strcmp(c->brand_string, CPUID_VENDOR_QEMU) || !strcmp(c->brand_string, CPUID_VENDOR_KVM)) {
		cpu_set_cap(c, X86_FEATURE_INVARIANT_TSC);
	}

	if (cpu_has(c, X86_FEATURE_INVARIANT_TSC)) {
		cpu_set_cap(c, X86_FEATURE_CONSTANT_TSC);
		cpu_set_cap(c, X86_FEATURE_NONSTOP_TSC);
	}
}
