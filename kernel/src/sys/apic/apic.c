#include "arch/x86/mem/paging.h"
#include "arch/x86/mem/paging_common.h"
#include "arch/x86/ports.h"
#include "arch/x86/registers.h"
#include "arch/x86/idt.h"
#include "io/logging.h"
#include "sys/acpi.h"
#include "arch/x86/msr.h"
#include "arch/x86/pic.h"
#include "sys/apic.h"
#include "sys/ioapic.h"
#include "sys/lapic.h"
#include "sys/rsdt.h"
#include <common.h>

size_t lapic_addr = 0;

bool volatile __using_apic = false;

// {
//     uint32_t eax, edx;

//     rdmsr(INTEL_APIC_BASE_MSR, eax, edx);

//     qemu_log("APIC BASE: %x", eax & 0xfffff000);
// }

uint32_t apic_write(uint32_t reg, uint32_t value) {
    return *(uint32_t volatile*)(lapic_addr + reg) = value;
}

uint32_t apic_read(uint32_t reg) {
    return *((uint32_t volatile*)(lapic_addr + reg));
}

void apic_init() {
    if(!apic_find_ioapic()) {
        qemu_err("Could not initialize IOAPIC, bailing out...");
        return;
    }

    // RSDPDescriptor *rsdp = acpi_rsdp_find();
    // acpi_find_apic(rsdp->RSDTaddress, &lapic_addr);
    // qemu_log("LAPIC: %x", lapic_addr);

    // Disable old PIC
    pic_disable();

    uint32_t eax, edx;

    rdmsr(INTEL_APIC_BASE_MSR, eax, edx);

    lapic_addr = eax & 0xfffff000;

	// Enable APIC by setting 11th bit, and settijng bootstrap processor by setting 8th bit
    wrmsr(INTEL_APIC_BASE_MSR, (lapic_addr & 0xfffff000) | (1 << 8) | (1 << 11), 0);

    qemu_log("APIC BASE: %x", lapic_addr);

    map_pages(
        get_kernel_page_directory(), 
        lapic_addr, 
        lapic_addr, 
        PAGE_SIZE, 
        PAGE_WRITEABLE | PAGE_CACHE_DISABLE
    );

    apic_write(0xF0, 0x1FF);

    size_t ver = ioapic_read(IOAPIC_REG_VER) & 0xff;

    qemu_log("I/O APIC VER: 0x%x", ver);

    //ioapic_write(IOAPIC_REG_ID, 0);

    // for(register int i = 0; i < 16; i++) {
    //     ioapic_write(IOAPIC_REG_REDTBL_BASE + (i * 2), (32 + i));
    //     ioapic_write(IOAPIC_REG_REDTBL_BASE + (i * 2) + 1, 0);
    // }

    ioapic_post_initialize();

    qemu_log("IOAPIC INITIALIZED!");

    __using_apic = true;

    // lapic_init();

    // qemu_log("LAPIC INITIALIZED!");
}