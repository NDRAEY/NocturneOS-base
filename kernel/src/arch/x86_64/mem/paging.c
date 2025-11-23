#include "io/logging.h"
#include "mem/pmm.h"
#include <common.h>
#include <arch/x86/mem/paging_common.h>
#include <arch/x86/mem/paging.h>
#include <multiboot.h>

extern size_t KERNEL_BASE_pos;
extern size_t KERNEL_END_pos;

#define PML4T_IDX(addr) (((addr) >> 39) & 0x1ff)
#define PDPT_IDX(addr) (((addr) >> 30) & 0x1ff)
#define PD_IDX(addr) (((addr) >> 21) & 0x1ff)
#define PT_IDX(addr) (((addr) >> 12) & 0x1ff)

size_t* get_kernel_page_directory() {
    return (size_t*)0xfffffffffffff000;
}

static size_t map_new_pd(page_directory_t* page_dir, virtual_addr_t virtual) {
    size_t pml4t_idx = PML4T_IDX(virtual);
    size_t pdpt_idx = PDPT_IDX(virtual);
    
    size_t addr = phys_alloc_single_page();
    
    qemu_log("Mapping page: %x", (uint32_t)addr);
   
    uint64_t* pdpt_addr = (uint64_t*)(page_dir[pml4t_idx] & ~0x3ff);

    pdpt_addr[pdpt_idx] = addr | PAGE_WRITEABLE | PAGE_PRESENT;

    uint64_t* pd = (uint64_t*)addr;

    // Recursive paging.
    pd[511] = addr | PAGE_PRESENT | PAGE_WRITEABLE;

    return addr;
}

static size_t map_new_pt(page_directory_t* page_dir, virtual_addr_t virtual) {
    size_t pml4t_idx = PML4T_IDX(virtual);
    size_t pdpt_idx = PDPT_IDX(virtual);
    size_t pd_idx = PD_IDX(virtual);
    
    size_t addr = phys_alloc_single_page();
    
    qemu_log("Mapping page: %x", (uint32_t)addr);
   
    uint64_t* pdpt_addr = (uint64_t*)(page_dir[pml4t_idx] & ~0x3ff);
    uint64_t* pdt_addr = (uint64_t*)(pdpt_addr[pdpt_idx] & ~0x3ff);

    pdt_addr[pd_idx] = addr | PAGE_WRITEABLE | PAGE_PRESENT;

    uint64_t* pt = (uint64_t*)addr;

    // Recursive paging.
    pt[511] = addr | PAGE_PRESENT | PAGE_WRITEABLE;

    return addr;
}

uint32_t phys_get_page_data(const page_directory_t* page_dir, virtual_addr_t virtual) {
    // qemu_log("! Map P%x => V%x", physical, virtual);

    size_t pml4t_idx = PML4T_IDX(virtual);
    size_t pdpt_idx = PDPT_IDX(virtual);
    size_t pd_idx = PD_IDX(virtual);
    size_t pt_idx = PT_IDX(virtual);

    if((page_dir[pml4t_idx] & 1) == 0) {
        qemu_log("PML4T[%d] is not mapped!", pml4t_idx);

        return 0;
    }

    uint64_t* pdpt_addr = (uint64_t*)(page_dir[pml4t_idx] & ~0x3ff);

    if((pdpt_addr[pdpt_idx] & 1) == 0) {
        qemu_log("PDPT[%d] is not mapped!", pdpt_idx);

        return 0;
    }

    uint64_t* pdt_addr = (uint64_t*)(pdpt_addr[pdpt_idx] & ~0x3ff);

    if((pdt_addr[pd_idx] & 1) == 0) {
        qemu_log("PD[%d] is not mapped!", pd_idx);

        return 0;
    }

    uint64_t* pt_addr = (uint64_t*)(pdt_addr[pd_idx] & ~0x3ff);

    return pt_addr[pt_idx];
}

size_t virt2phys(const page_directory_t *page_dir, virtual_addr_t virtual) {
	return phys_get_page_data(page_dir, virtual) & ~0x3ff;
}

void map_single_page(page_directory_t* page_dir, physical_addr_t physical, virtual_addr_t virtual, uint32_t flags) {
    // qemu_log("! Map P%x => V%x", physical, virtual);

    size_t pml4t_idx = PML4T_IDX(virtual);
    size_t pdpt_idx = PDPT_IDX(virtual);
    size_t pd_idx = PD_IDX(virtual);
    size_t pt_idx = PT_IDX(virtual);

    if((page_dir[pml4t_idx] & 1) == 0) {
        qemu_log("PML4T[%d] is not mapped!", pml4t_idx);
    }

    uint64_t* pdpt_addr = (uint64_t*)(page_dir[pml4t_idx] & ~0x3ff);

    if((pdpt_addr[pdpt_idx] & 1) == 0) {
        qemu_log("PDPT[%d] is not mapped!", pdpt_idx);

        map_new_pd(page_dir, virtual);
    }

    uint64_t* pdt_addr = (uint64_t*)(pdpt_addr[pdpt_idx] & ~0x3ff);

    if((pdt_addr[pd_idx] & 1) == 0) {
        qemu_log("PD[%d] is not mapped!", pd_idx);

        map_new_pt(page_dir, virtual);
    }

    uint64_t* pt_addr = (uint64_t*)(pdt_addr[pd_idx] & ~0x3ff);

    pt_addr[pt_idx] = physical | flags | PAGE_PRESENT;
    // qemu_log("%d %d %d %d", pml4t_idx, pdpt_idx, pd_idx, pt_idx);
}

void unmap_single_page(page_directory_t* page_dir, virtual_addr_t virtual) {
    // qemu_log("! Map P%x => V%x", physical, virtual);

    size_t pml4t_idx = PML4T_IDX(virtual);
    size_t pdpt_idx = PDPT_IDX(virtual);
    size_t pd_idx = PD_IDX(virtual);
    size_t pt_idx = PT_IDX(virtual);

    if((page_dir[pml4t_idx] & 1) == 0) {
        qemu_log("PML4T[%d] is not mapped!", pml4t_idx);
    }

    uint64_t* pdpt_addr = (uint64_t*)(page_dir[pml4t_idx] & ~0x3ff);

    if((pdpt_addr[pdpt_idx] & 1) == 0) {
        qemu_log("PDPT[%d] is not mapped!", pdpt_idx);

        map_new_pd(page_dir, virtual);
    }

    uint64_t* pdt_addr = (uint64_t*)(pdpt_addr[pdpt_idx] & ~0x3ff);

    if((pdt_addr[pd_idx] & 1) == 0) {
        qemu_log("PD[%d] is not mapped!", pd_idx);

        map_new_pt(page_dir, virtual);
    }

    uint64_t* pt_addr = (uint64_t*)(pdt_addr[pd_idx] & ~0x3ff);

    pt_addr[pt_idx] = 0;
}

extern uint64_t _pdt[512];

void paging_preinit(const multiboot_header_t* mboot) {
    size_t kstart = (size_t)&KERNEL_BASE_pos;
    size_t grub_last_module_end = (((const multiboot_module_t *)mboot->mods_addr) + (mboot->mods_count - 1))->mod_end;
    size_t kend = grub_last_module_end + PAGE_BITMAP_SIZE;

    size_t big_page_count = ALIGN(kend - kstart, PAGE_SIZE * 512) / (PAGE_SIZE * 512);

    if(big_page_count > 512) {
        // panic()
    }

    qemu_log("Will use %d big pages", big_page_count);

    for(size_t i = 0; i < big_page_count; i++) {
        _pdt[i] = (0x200000ull * i) | PAGE_PRESENT | PAGE_WRITEABLE | PAGE_EXTENDED;
    }

    __asm__ volatile("mov %%cr3, %%rax\nmov %%rax, %%cr3" ::: "memory");

    qemu_log("Kernel %x - %x (%d bytes)", kstart, kend, kend - kstart);
}