#include <common.h>
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