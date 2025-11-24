#include <common.h>
#include <io/logging.h>
#include <arch/x86/mem/paging_common.h>

/**
 * @brief Map pages
 *
 * @param page_dir Page directory address
 * @param physical Address of physical memory
 * @param virtual Address of virtual memory
 * @param size Amount of BYTES to map (must be aligned by 4096)
 * @param flags Page flags
 */
 void map_pages(page_directory_t* page_dir, physical_addr_t physical, virtual_addr_t virtual, size_t size, uint32_t flags) {	
	physical_addr_t phys = physical;
	physical_addr_t virt = virtual;

	for(virtual_addr_t vend = ALIGN(virt + size, PAGE_SIZE);
	    virt <= vend;
	    phys += PAGE_SIZE,
	    virt += PAGE_SIZE
	) {
			map_single_page(page_dir, phys, virt, flags);
	}

	reload_cr3();
}

/**
 * @brief Map pages (but physical address can be unaligned)
 *
 * @param page_dir Page directory address
 * @param physical Address of physical memory
 * @param virtual Address of virtual memory
 * @param size Amount of BYTES to map (can be without align, if you want)
 * @param flags Page flags
 */
 void map_pages_overlapping(page_directory_t* page_directory, size_t physical_start, size_t virtual_start, size_t size, uint32_t flags) {
    // Explanation: We want to map address 0xd000abcd with size 2345
    // If we will use map_pages it will map only one page, because addresses gets aligned to PAGE_SIZE
    // (0xd000abcd -> 0xd000a000), and size too (2345 -> 4096)
    // So it uses memory from 0xd000abcd to 0xd000b4f6 (2 pages)
    //
    // We need to calculate how many pages we need to map
	// size_t pages_to_map = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    // And then map them

    size_t nth1 = virtual_start / PAGE_SIZE;
    size_t nth2 = (virtual_start + size) / PAGE_SIZE;

    size_t pages_to_map = (nth2 - nth1) + 1;

    // qemu_log("Range: %x - %x", virtual_start, virtual_start + size);

    qemu_note("Mapping %u pages to %x", pages_to_map, physical_start);
    map_pages(page_directory, physical_start, virtual_start, pages_to_map * PAGE_SIZE, flags);
}

void unmap_pages_overlapping(page_directory_t* page_directory, size_t virtual, size_t size) {
    virtual &= ~0xfff;

    size_t nth1 = virtual / PAGE_SIZE;
    size_t nth2 = (virtual + size) / PAGE_SIZE;

    size_t pages_to_map = (nth2 - nth1) + 1;

    for(size_t i = 0; i < pages_to_map; i++) {
        unmap_single_page(page_directory, virtual + (i * PAGE_SIZE));
    }
}
