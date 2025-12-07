#include <common.h>
#include <sys/apic.h>
#include <sys/ioapic.h>
#include <sys/acpi.h>
#include <io/logging.h>
#include <arch/x86/mem/paging_common.h>

static size_t ioapic_addr;

bool apic_find_ioapic() {
    RSDPDescriptor *rsdp = acpi_rsdp_find();

    if(!rsdp) {
        return false;
    }

    map_pages_overlapping(
        get_kernel_page_directory(), 
        rsdp->RSDTaddress,
        rsdp->RSDTaddress,
        PAGE_SIZE, 
        0
    );

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

    qemu_log("IOAPIC: %x", ioapic_addr);

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