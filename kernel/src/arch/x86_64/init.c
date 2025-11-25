#include <common.h>
#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>
#include "arch/x86/mem/paging_common.h"
#include "arch/x86_64/idt64.h"
#include "arch/x86/pit.h"
#include "arch/x86/isr.h"
#include "arch/x86_64/registers64.h"
#include "arch/x86_64/mem/paging.h"
#include "io/logging.h"
#include "mem/vmm.h"
#include "sys/apic.h"
#include "sys/grub_modules.h"
#include <mem/pmm.h>
#include <lib/string.h>
#include <multiboot.h>

// void _tty_printf() {}


void __attribute__((noreturn)) arch_init(const multiboot_header_t *mboot) {
    qemu_log("Hello, world! (MBOOT = %x)", (uint32_t)(size_t)mboot);

    init_idt();
    isr_init();
    
    check_memory_map((const memory_map_entry_t*)mboot->mmap_addr, mboot->mmap_length);
    mark_reserved_memory_as_used((const memory_map_entry_t*)mboot->mmap_addr, mboot->mmap_length);
    
    paging_preinit(mboot);
    
    grub_modules_prescan(mboot);
    
    init_pmm(mboot);
    
    mark_reserved_memory_as_used((memory_map_entry_t *)mboot->mmap_addr, mboot->mmap_length);

    qemu_ok("Physical memory manager and paging initialized!");

    // vmm_init();

    // qemu_ok("Heap initialized!");

    __asm__ volatile("cli");
    apic_init();

    init_timer(1000);

    // STI called after init_timer().

    size_t screen_size = mboot->framebuffer_pitch * mboot->framebuffer_height;

    qemu_log("[%x]: Screen size: %d bytes", mboot->framebuffer_addr, screen_size);

    map_pages(
        get_kernel_page_directory(), 
        mboot->framebuffer_addr,
        mboot->framebuffer_addr,
        screen_size,
        PAGE_WRITEABLE | PAGE_CACHE_DISABLE
    );

    memset((void*)mboot->framebuffer_addr, 0x35, screen_size);
    
    while(1)
    ;
    
    qemu_log("OK!");

    while(1)
        ;
}
