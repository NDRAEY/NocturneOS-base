// Charmander - a new virtual memory manager by NDRAEY (c) 2023
// for SayoriOS
//
// Created by ndraey on 05.11.23.
//

#pragma once

#include <common.h>
#include "mem/pmm.h"
#include "lib/string.h"

// #define LAZY_KREALLOC

struct heap_entry {
	size_t address;
	size_t length;
};

typedef struct heap {
	size_t allocated_count;
	size_t capacity; // Entries
	size_t start;
	size_t used_memory;
	struct heap_entry* memory;
} heap_t;

extern heap_t system_heap;

void vmm_init();
void *alloc_no_map(size_t size, size_t align);
void free_no_map(void* ptr);
bool vmm_is_page_used_by_entries(size_t address);
void* kmalloc_common(size_t size, size_t align)  __attribute__((__malloc__)) __attribute__((__alloc_size__(1)));
void *kmalloc_common_contiguous(physical_addr_t* page_directory, size_t page_count);

SAYORI_INLINE void* kmalloc(size_t size) {
	return kmalloc_common(size, 1);
}

void* krealloc(void* ptr, size_t memory_size);
void kfree(void* ptr);
void* clone_kernel_page_directory(size_t virts_out[1024]);

void vmm_debug_switch(bool enable);

SAYORI_INLINE void* kcalloc(size_t size, size_t amount) {
	void* x = kmalloc(size * amount);

	memset(x, 0, size * amount);

	return x;
}

void heap_dump();