#pragma once

#include <arch/x86/mem/paging_common.h>
#include <multiboot.h>

extern void load_page_directory(size_t addr);
extern void enable_paging();

void blank_page_directory(page_directory_t* pagedir_addr);

size_t phys_get_page_data(page_directory_t* page_dir, virtual_addr_t virtual);
size_t virt2phys(const page_directory_t *page_dir, virtual_addr_t virtual);
void init_paging(const multiboot_header_t *mboot);

void map_pages_overlapping(page_directory_t* page_directory, size_t physical_start, size_t virtual_start, size_t size, uint32_t flags);
void unmap_pages_overlapping(page_directory_t* page_directory, size_t virtual, size_t size);
void phys_set_flags(page_directory_t* page_dir, virtual_addr_t virtual, uint32_t flags);

void premap_pages(page_directory_t* page_dir, physical_addr_t physical, virtual_addr_t virtual, size_t size);
size_t virt2phys_precise(const page_directory_t *page_dir, virtual_addr_t virtual);

size_t virt2phys_ext(const page_directory_t *page_dir, const size_t* virts, virtual_addr_t virtual);
