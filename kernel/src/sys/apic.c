#include "arch/x86/mem/paging_common.h"
#include "arch/x86/ports.h"
#include "arch/x86/registers.h"
#include "arch/x86/idt.h"
#include "io/logging.h"
#include "sys/acpi.h"
#include "arch/x86/msr.h"
#include "sys/apic_table.h"
#include "sys/ioapic.h"
#include "sys/rsdt.h"
#include <common.h>

static size_t lapic_addr;
static size_t ioapic_addr;

bool __using_apic = false;

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

void spurious_vec_handler(registers_t registers) {
    qemu_log("SPURIOUS!");
}

bool apic_find_ioapic() {
    RSDPDescriptor *rsdp = acpi_rsdp_find();
    qemu_log("RSDP FOUND!");

    map_pages(get_kernel_page_directory(), rsdp->RSDTaddress, rsdp->RSDTaddress, PAGE_SIZE, 0);

    uint32_t length = ((ACPISDTHeader*)(rsdp->RSDTaddress))->Length;
    uint32_t sdt_count = (length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);

    qemu_log("RSDT Length: %x", length);

    ACPISDTHeader* apic = acpi_find_table(rsdp->RSDTaddress, sdt_count, "APIC");

    if(apic == NULL) {
        return false;
    }

    size_t table_size = apic->Length - (sizeof(ACPISDTHeader) + sizeof(struct APIC_Base_Table));
    
    qemu_log("APIC Length: %d bytes (pure %d bytes)", apic->Length, table_size);

    size_t table = ((uint32_t)(apic) + sizeof(ACPISDTHeader) + sizeof(struct APIC_Base_Table));

    size_t offset = 0;

    while(offset < table_size) {
        struct APIC_Entry* entry = (struct APIC_Entry*)(table + offset);

        qemu_log("[APIC entry]: Type %d; Length: %d", entry->type, entry->record_length);

        if(entry->type == APIC_IOAPIC) {
            qemu_ok("Found IOAPIC @ %x!", entry->entry.ioapic.io_apic_address);

            map_pages(
                get_kernel_page_directory(), 
                entry->entry.ioapic.io_apic_address,
                entry->entry.ioapic.io_apic_address, 
                PAGE_SIZE,
                PAGE_WRITEABLE
            );

            ioapic_addr = entry->entry.ioapic.io_apic_address;
        } else if(entry->type == APIC_PLAPIC) {
            qemu_log(
                "PLAPIC: APIC ID: %x; Processor ID: %x; Flags: %x",
                entry->entry.plapic.apic_id,
                entry->entry.plapic.processor_id,
                entry->entry.plapic.flags
            );
        } else if(entry->type == APIC_IOAPIC_ISO) {
            qemu_log(
                "INTERRUPT SOURCE OVERRIDE: Bus: %x; GSI: %x; IRQ Source: %x; Flags: %x",
                entry->entry.ioapic_iso.bus_source,
                entry->entry.ioapic_iso.global_system_interrupt,
                entry->entry.ioapic_iso.irq_source,
                entry->entry.ioapic_iso.flags
            );
        }

        offset += entry->record_length;
    }

    return true;
}

uint32_t ioapic_read(uint32_t reg) {
    *((uint32_t volatile*)(ioapic_addr + IOAPIC_REGSEL)) = reg;

    return *((uint32_t volatile*)(ioapic_addr + IOAPIC_DATA));
}

void ioapic_write(uint32_t reg, uint32_t value) {
    *((uint32_t volatile*)(ioapic_addr + IOAPIC_REGSEL)) = reg;
    *((uint32_t volatile*)(ioapic_addr + IOAPIC_DATA)) = value;
}

void ioapic_post_initialize() {
    RSDPDescriptor *rsdp = acpi_rsdp_find();

    uint32_t length = ((ACPISDTHeader*)(rsdp->RSDTaddress))->Length;
    uint32_t sdt_count = (length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);

    ACPISDTHeader* apic = acpi_find_table(rsdp->RSDTaddress, sdt_count, "APIC");

    size_t table_size = apic->Length - (sizeof(ACPISDTHeader) + sizeof(struct APIC_Base_Table));
    
    size_t table = ((uint32_t)(apic) + sizeof(ACPISDTHeader) + sizeof(struct APIC_Base_Table));

    size_t offset = 0;

    while(offset < table_size) {
        struct APIC_Entry* entry = (struct APIC_Entry*)(table + offset);

        if(entry->type == APIC_IOAPIC_ISO) {
            size_t irq = entry->entry.ioapic_iso.irq_source;
            size_t gsi = entry->entry.ioapic_iso.global_system_interrupt;

            qemu_log(
                "Remapping: IRQ Source: %d -> GSI: %d",
                irq, gsi
            );

            ioapic_write(IOAPIC_REG_REDTBL_BASE + (gsi * 2), 32 + irq);
            ioapic_write(IOAPIC_REG_REDTBL_BASE + (gsi * 2) + 1, 0);
        }

        offset += entry->record_length;
    }

    // Let's fill IOAPIC REDTBL with identity values:
    
    for(register int i = 1; i < 24; i++) {
        size_t redtbl_i = ioapic_read(IOAPIC_REG_REDTBL_BASE + (i * 2));

        if(redtbl_i & 0x10000) {
            qemu_log("IDENTITY MAP: %d -> %d", i, i);

            ioapic_write(IOAPIC_REG_REDTBL_BASE + (i * 2), 32 + i);
            ioapic_write(IOAPIC_REG_REDTBL_BASE + (i * 2) + 1, 0);
        }
    }
}

void apic_init() {
    if(!apic_find_ioapic()) {
        qemu_err("Could not initialize IOAPIC, bailing out...");
        return;
    }

    qemu_log("IOAPIC: %x", ioapic_addr);

    // RSDPDescriptor *rsdp = acpi_rsdp_find();

    // acpi_find_apic(rsdp->RSDTaddress, &lapic_addr);

    // qemu_log("LAPIC: %x", lapic_addr);

    // Disable old PIC
    outb(0xA0, 0x11);
    outb(0x20, 0x10);
    outb(0xA1, 0x20);
    outb(0x21, 0x28);
    outb(0xA1, 0x2);
    outb(0x21, 0x4);
    outb(0xA1, 0x01);
    outb(0x21, 0x01);
    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);

    uint32_t eax, edx;

    rdmsr(INTEL_APIC_BASE_MSR, eax, edx);

    lapic_addr = eax & 0xfffff000;

    wrmsr(INTEL_APIC_BASE_MSR, (lapic_addr & 0xfffff000) | 0x100 | (1 << 11), 0);

    qemu_log("APIC BASE: %x", lapic_addr);

    map_pages(
        get_kernel_page_directory(), 
        lapic_addr, 
        lapic_addr, 
        PAGE_SIZE, 
        PAGE_WRITEABLE | PAGE_CACHE_DISABLE
    );

    apic_write(0xF0, 0x1FF);
    // apic_write(0xE0, 0xFFFFFFFF);
    // apic_write(0x80, 0);

    size_t ver = ioapic_read(IOAPIC_REG_VER) & 0xff;

    qemu_log("I/O APIC VER: 0x%x", ver);

    //ioapic_write(IOAPIC_REG_ID, 0);

    // for(register int i = 0; i < 16; i++) {
    //     ioapic_write(IOAPIC_REG_REDTBL_BASE + (i * 2), (32 + i));
    //     ioapic_write(IOAPIC_REG_REDTBL_BASE + (i * 2) + 1, 0);
    // }

    ioapic_post_initialize();

    qemu_log("INITIALIZED!");

    __using_apic = true;
}
