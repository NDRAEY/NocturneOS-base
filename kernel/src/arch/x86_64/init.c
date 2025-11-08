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
#include <mem/pmm.h>
#include <lib/string.h>
#include <multiboot.h>

void __attribute__((noreturn)) arch_init(const multiboot_header_t *mboot) {
    qemu_log("Hello, world! (MBOOT = %x)", (uint32_t)(size_t)mboot);

    init_idt();
    isr_init();

    init_timer(1000);

    check_memory_map((const memory_map_entry_t*)mboot->mmap_addr, mboot->mmap_length);
    mark_reserved_memory_as_used((const memory_map_entry_t*)mboot->mmap_addr, mboot->mmap_length);

    paging_preinit(mboot);
    init_pmm(mboot);
    
    size_t screen_size = mboot->framebuffer_pitch * mboot->framebuffer_height;

    qemu_log("Screen size: %d bytes", screen_size);

    map_pages(
        get_kernel_page_directory(), 
        mboot->framebuffer_addr,
        mboot->framebuffer_addr,
        screen_size,
        PAGE_WRITEABLE
    );

    qemu_log("OK!");
    
    memset((void*)mboot->framebuffer_addr, 0xff, screen_size);

    while(1)
        ;
}
