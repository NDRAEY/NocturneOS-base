#include <common.h>
#include <arch/x86/mem/paging.h>
#include <arch/x86/mem/paging_common.h>

#include <mem/pmm.h>
#include <lib/string.h>
#include <io/logging.h>

extern size_t KERNEL_BASE_pos;
extern size_t KERNEL_END_pos;

physical_addr_t* kernel_page_directory = 0;
bool paging_initialized = false;

// Creates and prepares a page directory
uint32_t * new_page_directory() {
	// Allocate a page (page directory is 4096 bytes)
	size_t* dir = (size_t*)phys_alloc_single_page();

	qemu_log("Allocated page directory at: %p", dir);

	// Blank it (they can store garbage, so we need to blank it)
	memset(dir, 0, PAGE_SIZE);

	qemu_log("Blanked directory.");

    // Recursive paging.
	dir[1023] = (uint32_t)dir | 3;

	qemu_log("================ Page directory is ready.");

	return dir;
}

uint32_t* get_page_table_by_vaddr(const page_directory_t* page_dir, virtual_addr_t vaddr) {
	if(paging_initialized)
		return (page_directory_t*)((char*)page_directory_start + (PD_INDEX(vaddr) * PAGE_SIZE));
	else
		return (page_directory_t*)(page_dir[PD_INDEX(vaddr)] & ~0xfff);
}

// Maps a page.
// Note: No need to set PAGE_PRESENT flag, it sets automatically.

/// \brief Maps physical page with virtual address space
/// \param page_dir VIRTUAL address of page directory
/// \param physical PHYSICAL address to map
/// \param virtual VIRTUAL address to map
/// \param flags Page flags (PAGE_PRESENT is automatically included)
void map_single_page(page_directory_t* page_dir, physical_addr_t physical, virtual_addr_t virtual, uint32_t flags) {
	// Clean flags and some garbage from addresses.

	virtual &= ~0xfff;
	physical &= ~0xfff;

//	qemu_log("V%x => P%x", virtual, physical);

	// Get our Page Directory Index and Page Table Index.
	uint32_t pdi = PD_INDEX(virtual);
	uint32_t pti = PT_INDEX(virtual);

	uint32_t* pt;

	// Check if page table not present.
	if((page_dir[pdi] & 1) == 0) {
		pt = (uint32_t *)phys_alloc_single_page();

		// FIXME: Is it safe to make page tables accessible by USER?
		// I added the USER-flag, because of #PF in usermode no matter the PTE is set with USER-flag
		uint32_t entry = (uint32_t)pt | PAGE_WRITEABLE | PAGE_PRESENT;

		if(flags & PAGE_USER) {
			entry |= PAGE_USER;
		}

		page_dir[pdi] = entry;

		if(paging_initialized && page_dir == get_kernel_page_directory()) {
			uint32_t pt_addr = (uint32_t)page_directory_start + (pdi * PAGE_SIZE);

			memset((uint32_t*)pt_addr, 0, PAGE_SIZE);

			pt = (uint32_t*)pt_addr;
		} else if(paging_initialized && page_dir != get_kernel_page_directory()) {
			qemu_warn("FIXME: Mapping other page directories");
			while(1);
		} else {
            memset(pt, 0, PAGE_SIZE);
		}
	} else {
		// Just get our page table
		pt = get_page_table_by_vaddr(page_dir, virtual);
	}

//	qemu_log("P: %x | V: %x => PDI: %d | PTI: %d", physical, virtual, pdi, pti);

	// Finally map our physical page to virtual
	pt[pti] = physical | flags | PAGE_PRESENT;

	// Do our best to take effect.
	reload_cr3();
	__asm__ volatile ("invlpg (,%0,)"::"a"(virtual));
}

void unmap_single_page(page_directory_t* page_dir, virtual_addr_t virtual) {
	virtual &= ~0xfff;
	
	uint32_t* pt;
	
	// Check if page table not present.
	if((page_dir[PD_INDEX(virtual)] & 1) == 0) {
		return;
	} else {
//		qemu_log("Page table exist");
		pt = get_page_table_by_vaddr(page_dir, virtual);
	}

//	qemu_log("Got page table at: %x", pt);
//	qemu_log("Unmapping: %x", virtual);

	pt[PT_INDEX(virtual)] = 0;

	reload_cr3();
}

uint32_t phys_get_page_data(const page_directory_t* page_dir, virtual_addr_t virtual) {
	virtual &= ~0x3ff;
	
	uint32_t* pt;
	
	// Check if page table not present.
	if((page_dir[PD_INDEX(virtual)] & 1) == 0) {
		return 0;
	} else {
		pt = get_page_table_by_vaddr(page_dir, virtual);
	}

	return pt[PT_INDEX(virtual)];
}

size_t virt2phys(const page_directory_t *page_dir, virtual_addr_t virtual) {
	if(page_dir == NULL) {
		return 0;
	}

	virtual &= ~0xfff;

	uint32_t* pt;
	
	// Check if page table not present.
	if((page_dir[PD_INDEX(virtual)] & 1) == 0) {
		return 0;
	} else {
		pt = get_page_table_by_vaddr(page_dir, virtual);
	}

	return pt[PT_INDEX(virtual)] & ~0x3ff;
}

uint32_t virt2phys_precise(const page_directory_t *page_dir, virtual_addr_t virtual) {
	if(page_dir == NULL) {
		return 0;
	}
	
	size_t phys = virt2phys(page_dir, virtual & ~0xfff);

	return phys + (virtual & 0xfff);
}

size_t virt2phys_ext(const page_directory_t *page_dir, const uint32_t* virts, virtual_addr_t virtual) {
	if(page_dir == NULL) {
		return 0;
	}

	virtual &= ~0xfff;

	uint32_t* pt;
	
	// Check if page table not present.
	if((page_dir[PD_INDEX(virtual)] & 1) == 0) {
		return 0;
	} else {
		pt = (uint32_t*)(virts[PD_INDEX(virtual)]);
	}

	return pt[PT_INDEX(virtual)] & ~0x3ff;
}

void phys_set_flags(page_directory_t* page_dir, virtual_addr_t virtual, uint32_t flags) {
    virtual &= ~0xfff;

    uint32_t* pt;

    // Check if page table not present.
    if((page_dir[PD_INDEX(virtual)] & 1) == 0) {
        return;
    } else {
        pt = get_page_table_by_vaddr(page_dir, virtual);
    }

	// Duplicated from `map_single_page()`.
	//
	// FIXME: Is it safe to make page table entries accessible by USER?
	// I added the USER-flag, because of #PF in usermode no matter the PTE is set with USER-flag
	if(flags & PAGE_USER) {
		page_dir[PD_INDEX(virtual)] |= PAGE_USER;
	}

    pt[PT_INDEX(virtual)] = (pt[PT_INDEX(virtual)] & ~0x3ff) | flags | PAGE_PRESENT;
}

size_t* get_kernel_page_directory() {
	if(paging_initialized)
		return page_directory_virt;
	else
		return kernel_page_directory;
}

extern size_t grub_last_module_end;

void init_paging(const multiboot_header_t *mboot) {	
	size_t grub_last_module_end = (((const multiboot_module_t *)mboot->mods_addr) + (mboot->mods_count - 1))->mod_end;
	size_t real_end = (size_t)(grub_last_module_end + PAGE_BITMAP_SIZE);

	// Create new page directory

	kernel_page_directory = (physical_addr_t*)new_page_directory();

	qemu_log("New page directory at: %x", (size_t)kernel_page_directory);

	qemu_log("Map (P/V) from %x to %x (%d bytes)", kernel_start, real_end, real_end - kernel_start);

	map_pages(
		kernel_page_directory,
		kernel_start,
		kernel_start,
		real_end - kernel_start,
		PAGE_WRITEABLE
	);

	// TODO: Detect mboot sizes correctly
	map_pages(
		kernel_page_directory,
		(size_t)mboot,
		(size_t)mboot,
		PAGE_SIZE,
		PAGE_WRITEABLE
	);

	qemu_log("Physical memory size: %x", phys_memory_size);

	load_page_directory((size_t) kernel_page_directory);

	qemu_log("Ok?");

	enable_paging();

	paging_initialized = true;

	// Here paging enabled and every memory error will lead to a Page Fault

	uint32_t* pd = get_kernel_page_directory();

	for(int i = 0; i < 1024; i++) {
		if(pd[i] != 0) {
			qemu_log("[%d]: %x", i, pd[i]);
        }
	}
}