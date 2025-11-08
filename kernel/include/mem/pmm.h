#pragma once

// Scyther Physical Memory Manager by NDRAEY (c) 2023
// for SayoriOS

#include	"common.h"
#include	"multiboot.h"

#ifdef NOCTURNE_X86
#include	"arch/x86/mem/paging_common.h"
#include	"arch/x86/mem/paging.h"
#endif

#ifdef NOCTURNE_X86_64
#include	"arch/x86/mem/paging_common.h"
#endif

extern size_t phys_memory_size;
extern size_t used_phys_memory_size;

extern size_t kernel_start;
extern size_t kernel_end;

void init_pmm(const multiboot_header_t* hdr);

physical_addr_t phys_alloc_single_page();
physical_addr_t phys_alloc_multi_pages(size_t count);

void phys_free_single_page(physical_addr_t addr);
void phys_free_multi_pages(physical_addr_t addr, size_t count);

void phys_not_enough_memory();

bool phys_is_used_page(physical_addr_t addr);
void phys_mark_page_entry(physical_addr_t addr, bool used);

void check_memory_map(const memory_map_entry_t* mmap_addr, uint32_t length);
void mark_reserved_memory_as_used(const memory_map_entry_t* mmap_addr, uint32_t length);
