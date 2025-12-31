/**
 * @brief Менеджер физической памяти
 * @author NDRAEY >_
 * @version 0.4.3
 * @date 2023-11-04
 * @copyright Copyright SayoriOS Team (c) 2022-2026
 */

// Scyther Physical Memory Manager by NDRAEY (c) 2023
// for SayoriOS

#include <common.h>
#include "lib/math.h"
#include "mem/pmm.h"
#include  "arch/x86/ports.h"
#include "io/logging.h"
#include "multiboot.h"
#include "sys/scheduler.h"

extern size_t KERNEL_BASE_pos;
extern size_t KERNEL_END_pos;

size_t kernel_start;
size_t kernel_end;

size_t mmap_length = 0;

size_t phys_memory_size = 0;
size_t used_phys_memory_size = 0;

/// Карта занятых страниц
uint8_t* pages_bitmap = 0;

/**
 * @brief Allocates a single page (4096 bytes)
 * @return Physical address of page
 */
physical_addr_t phys_alloc_single_page() {
	if(used_phys_memory_size >= phys_memory_size) {
		// If no free space, just call the function that handles that situation.
		qemu_log("No free physical memory. Running emergency scenario...");

		phys_not_enough_memory();
	}

	for(size_t i = 0; i < phys_get_bitmap_size(); i++) {
		if(pages_bitmap[i] == 0xff) {
			// 0xff is eight ones in 8 bit.
			// 8 ones - all pages in this index is used.
			continue;
		} else {
			// Otherwise, we have some bits cleared - we have free page(s).
			// Roll over all bits
			for(int j = 0; j < 8; j++) {
				// If we have bit cleared, mark it as used, increment memory stat and return address.
				if(((pages_bitmap[i] >> j) & 1) == 0) {
					// Page is free
					pages_bitmap[i] |= (1 << j);
					
					used_phys_memory_size += PAGE_SIZE;
					
					// Every (8bit) entry can handle (4096 * 8) = 32768 bytes.
					// Every bit of entry can hold one page (4096 bytes). 
					return (PAGE_SIZE * 8 * i) + (j * PAGE_SIZE);
				}
			}
		}
	}

	return 0;
}

/**
 * @brief Выделяет несколько страниц последовательно
 * @param count количество страниц
 * @return Физический адрес где начинаются страницы
 */
physical_addr_t phys_alloc_multi_pages(size_t count) {
	if(used_phys_memory_size + (count * PAGE_SIZE) >= phys_memory_size) {
		qemu_log("No free physical memory. Running emergency scenario...");

		phys_not_enough_memory();
	}

	size_t counter = 0;
	size_t addr = 0;

	// They used for saving start indexes of our pages.
	size_t si = 0, sj = 0;

	for(size_t i = 0; i < phys_get_bitmap_size(); i++) {
		if(pages_bitmap[i] == 0xff) {
			// 0xff is eight ones in 8 bit.
			// 8 ones - all pages in this index is used.
			continue;
		} else {
			// Otherwise, we have some bits cleared - we have free page(s).
			// Roll over all bits
			for(size_t j = 0; j < 8; j++) {
				// Check if page is free
				if(((pages_bitmap[i] >> j) & 1) == 0) {
					// If we starting, we need to save an address.
					if(counter == 0) {
						si = i;
						sj = j;
						addr = (PAGE_SIZE * 8 * i) + (j * PAGE_SIZE);
					} else if(counter == count) {
						// If we found `count` free pages in a row, we should mark them as used and return address.

						// Roll through all entries, starting from indices we preserved.
						for(; si < phys_get_bitmap_size(); si++) {
							for(; sj < 8; sj++) {
								// Mark as used.
								pages_bitmap[si] |= (1 << sj);

								// We have no control, so keep loops running until we mark all `count` pages as used.
								// If we marked all pages, exit the loops.
								if(!--counter) {
									goto phys_multialloc_end;
								}
							}

							sj = 0;
						}

						phys_multialloc_end:

						used_phys_memory_size += PAGE_SIZE * count;

						return addr;
					}

					// We found a free page, so increment a counter
					counter++;
				} else {
					// Oh shit, we have encountered a used page! (Loud scream!)
					// Okay, just reset the counter and address to start from the beginning.

					counter = 0;
					addr = 0; // For sanity
				}
			}
		}
	}

	return 0;
}

/**
 * @brief Освобождает страницу физической памяти
 * @param addr Физический адрес страницы
 */
void phys_free_single_page(physical_addr_t addr) {
	if(!addr)
		return;

	// Extract our entry position (i) and bit in the entry (j).

	size_t i = addr / (PAGE_SIZE * 8);
	size_t j = (addr % (PAGE_SIZE * 8)) / PAGE_SIZE;

	// Just clear a nth bit
	pages_bitmap[i] &= ~(1 << j);

	used_phys_memory_size -= PAGE_SIZE;
}

/**
 * @brief освобождает несколько страниц подряд
 * @param addr Физический адрес где начинаются страницы
 * @param count Количество страниц
 */
void phys_free_multi_pages(physical_addr_t addr, size_t count) {
	if(!addr)
		return;

	// Extract our entry position (i) and bit in the entry (j).

	size_t i = addr / (PAGE_SIZE * 8);
	size_t j = (addr % (PAGE_SIZE * 8)) / PAGE_SIZE;

	// Roll over all entries starting from index of our address
	for(; i < phys_get_bitmap_size(); i++) {
		for(; j < 8; j++) {
			// Just clear a nth bit
			pages_bitmap[i] &= ~(1 << j);
		
			// If we freed all pages, just exit the function.
			if(!--count) {
				return;
			}
		}

		j = 0;
	}

	used_phys_memory_size -= PAGE_SIZE * count;
}

// Tells if page allocated there
bool phys_is_used_page(physical_addr_t addr) {
	if(!addr)
		return true;

	// Extract our entry position (i) and bit in the entry (j).

	size_t i = addr / (PAGE_SIZE * 8);
	size_t j = (addr % (PAGE_SIZE * 8)) / PAGE_SIZE;

	// Just clear a nth bit
	return (bool)((pages_bitmap[i] >> j) & 1);
}

// Marks page.
void phys_mark_page_entry(physical_addr_t addr, bool used) {
	if(!addr)
		return;

	if((addr / 4096) / 8 >= phys_get_bitmap_size()) {
		qemu_err("BUG: %zx is beyond bitmap!", addr);
		__asm__ volatile("cli \n hlt");
	}

	// Extract our entry position (i) and bit in the entry (j).

	size_t i = addr / (PAGE_SIZE * 8);
	size_t j = (addr % (PAGE_SIZE * 8)) / PAGE_SIZE;

	if(used)
		pages_bitmap[i] |= (1 << j);
	else
		pages_bitmap[i] &= ~(1 << j);
}

size_t getInstalledRam(){
    return phys_memory_size;
}

void mark_reserved_memory_as_used(const memory_map_entry_t* mmap_addr, uint32_t length) {
	size_t n = length / sizeof(memory_map_entry_t);

	for (size_t i = 0; i < n; i++) {
		const memory_map_entry_t* entry = mmap_addr + i;

		size_t addr = entry->addr_low;
		size_t length = entry->len_low;

		if(addr >= phys_get_bitmap_size() * 4096 * 8) {
			qemu_warn("Reserved entry is beyond memory bitmap range: %x", addr);
			continue;
		}

		if(entry->type != 1) {
			for(size_t j = 0; j < length; j += PAGE_SIZE) {
				phys_mark_page_entry(addr + j, 1);  // Mark as used
			}
		}
	}

	qemu_log("RAM: %d MB | %d KB | %d B", phys_memory_size/(1024*1024), phys_memory_size/1024, phys_memory_size);
}

void phys_not_enough_memory() {
	qemu_log("Not enough memory!");

	while(1);
}

void check_memory_map(const memory_map_entry_t* mmap_addr, uint32_t length){
	/* Entries number in memory map structure */
	mmap_length = length;
	size_t n = length / sizeof(memory_map_entry_t);

	/* Set pointer to memory map */
	
	qemu_log("[PMM] Map:");

	for (size_t i = 0; i < n; i++){
		const memory_map_entry_t* entry = mmap_addr + i;
		
		qemu_log("%s [Address: %x | Length: %x] <%d>",
				 (entry->type == 1 ? "Available" : "Reserved"),
				 entry->addr_low, entry->len_low, entry->type);

		phys_memory_size += entry->len_low;
	}

	qemu_log("RAM: %d MB | %d KB | %d B", phys_memory_size >> 20, phys_memory_size >> 10, phys_memory_size);
}

// Physical memory bitmap size, in bytes.
size_t bitmap_size = 0;

size_t phys_get_bitmap_size() {
	return bitmap_size;
}

void init_pmm(const multiboot_header_t* hdr) {
	// Calculate bitmap size
	size_t pages_available = ALIGN(phys_memory_size / PAGE_SIZE, 0x100);
	bitmap_size = pages_available / 8;

	qemu_log("Memory %zu bytes; %zu pages available", phys_memory_size, pages_available);
	qemu_log("Calculated bitmap size: %zu bytes", bitmap_size);

	size_t grub_last_module_end = (((const multiboot_module_t *)hdr->mods_addr) + (hdr->mods_count - 1))->mod_end;
	size_t real_end = (size_t)(grub_last_module_end + phys_get_bitmap_size());

	kernel_start = (size_t)&KERNEL_BASE_pos;
	kernel_end = (size_t)&KERNEL_END_pos;

    qemu_log("Last GRUB module ends at: %x", grub_last_module_end);

    pages_bitmap = (uint8_t*)grub_last_module_end;

	memset(pages_bitmap, 0, phys_get_bitmap_size());

	size_t kernel_size = real_end - kernel_start;

	qemu_log("Kernel starts at: %x", kernel_start);
	qemu_log("Kernel ends   at: %x (only kernel)", kernel_end);
	qemu_log("Kernel ends   at: %x (everything)", real_end);

	qemu_log("Kernel size is: %d (%d kB) (%d MB)", (kernel_end - kernel_start), (kernel_end - kernel_start) >> 10, (kernel_end - kernel_start) >> 20);
	qemu_log("Kernel size (initrd included) is: %d (%d kB) (%d MB)", kernel_size, kernel_size >> 10, kernel_size >> 20);

	kernel_size = ALIGN(kernel_size, PAGE_SIZE);

	// Preallocate our kernel space

	size_t page_count = ALIGN(real_end, PAGE_SIZE) / PAGE_SIZE;

	qemu_log("Allocating %d pages for kernel space...", page_count);

	{
		size_t reserved = phys_alloc_multi_pages(page_count);

		if(reserved != 0) {
			qemu_err("Should be 0, but got %x", reserved);
			qemu_err("Manual assertion failed: First physical memory allocation should return zero!");

			while(1)
			;
		}
	}

}