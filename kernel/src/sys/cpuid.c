//
// Created by ndraey on 03.10.23.
//

#include "common.h"
#include "sys/cpuid.h"
#include "io/ports.h"
#include "io/tty.h"

size_t cpu_get_id() {
	uint32_t ebx = 0, unused;

	cpuid(0, unused, ebx, unused, unused);

	return ebx;
}

bool is_long_mode_supported() {
	uint32_t eax = 0, unused = 0;

	cpuid(0x80000000, eax, unused, unused, unused);

	if(eax < 0x80000001) {
		return false;
	}

	return true;
}
