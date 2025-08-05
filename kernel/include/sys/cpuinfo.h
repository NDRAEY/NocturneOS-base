#pragma once

#include <common.h>

void get_cpu_brand(char* output, size_t* length_out);

SAYORI_INLINE uint64_t rdtsc() {
	uint32_t hi, lo;

	__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
	
	return ( (uint64_t)lo)|( ((uint64_t)hi) << 32 );
}

SAYORI_INLINE uint32_t rdtsc32() {
    uint32_t lo;

    __asm__ volatile("rdtsc" : "=a"(lo));

    return lo;
}
