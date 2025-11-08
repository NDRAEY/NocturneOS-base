#pragma once

#include <common.h>

#define		PAGE_SIZE				0x1000
#define		PAGE_OFFSET_MASK		0xFFF	
#define		PAGE_TABLE_INDEX_BITS	10	
#define		PAGE_TABLE_INDEX_MASK	0x3FF

#define		PAGE_PRESENT		(1U << 0)
#define		PAGE_WRITEABLE		(1U << 1)
#define		PAGE_USER			(1U << 2)
#define		PAGE_WRITE_THROUGH	(1U << 3)
#define		PAGE_CACHE_DISABLE	(1U << 4)
#define		PAGE_ACCESSED		(1U << 5)
#define		PAGE_DIRTY			(1U << 6)
#define		PAGE_EXTENDED		(1U << 7)
#define		PAGE_GLOBAL			(1U << 8)

#define     PAGE_BITMAP_SIZE (131072 * 4)

#define 	PD_INDEX(virt_addr) ((virt_addr) >> 22)
#define 	PT_INDEX(virt_addr) (((virt_addr) >> 12) & 0x3ff)

typedef 	size_t 			virtual_addr_t;
typedef 	size_t 			physical_addr_t;

typedef size_t pt_entry;
typedef size_t pd_entry;

typedef size_t page_directory_t;

#ifdef NOCTURNE_X86
// The space where first page table starts
/// Начало где расположены все таблицы страниц
#define page_directory_start ((uint32_t*)(0xffffffff - (4 * MB) + 1))

// The space where we can modify page directory
/// Начало директории таблиц для страниц
#define page_directory_virt ((uint32_t*)(0xffffffff - (4 * KB) + 1))
#endif

void map_single_page(page_directory_t* page_dir, physical_addr_t physical, virtual_addr_t virtual, uint32_t flags);
void unmap_single_page(page_directory_t* page_dir, virtual_addr_t virtual);
void map_pages(page_directory_t* page_dir, physical_addr_t physical, virtual_addr_t virtual, size_t size, uint32_t flags);

size_t* get_kernel_page_directory();

// This function is here, because both x86 and x86_64 implement it in their respective assemblies.
void reload_cr3();
