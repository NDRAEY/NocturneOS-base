#include "sys/acpi.h"
#include "sys/apic_table.h"
#include "sys/fadt.h"
#include "lib/string.h"
#include "io/logging.h"
#include "io/tty.h"
#include "mem/pmm.h"
#include "mem/vmm.h"

#define ACPI_PAGE_COUNT 32

uint32_t system_processors_found = 0;

extern uint16_t century_register;

RSDPDescriptor* acpi_rsdp_find() {
    map_pages(
        get_kernel_page_directory(),
        0xE0000, 
        0xE0000, 
        0x100000 - 0xE0000,
        0
    );

    size_t saddr = 0x000E0000;
    char rsdp_ptr[8] = {'R','S','D',' ','P','T','R',' '};

	for(; saddr < 0x000FFFFF; saddr++) {
		if(memcmp((const char*)saddr, (const char*)rsdp_ptr, 8) == 0) {
			qemu_log("Found! At: %x", saddr);
            break;
		}
	}

    RSDPDescriptor* rsdp = (RSDPDescriptor*)saddr;

    qemu_log("RSDP sig: %.8s", rsdp->signature);
    qemu_log("RSDP checksum: %d", rsdp->checksum);
    qemu_log("RSDP OEMID: %s", rsdp->OEMID);
    qemu_log("RSDP revision: %d", rsdp->revision);
    qemu_log("RSDT address: %x", rsdp->RSDTaddress);

	if(rsdp->RSDTaddress == 0) {
		return 0;
	}

    return rsdp;
}

bool acpi_checksum_sdt(const ACPISDTHeader *tableHeader) {
    unsigned char sum = 0;
 
    for (size_t i = 0; i < tableHeader->Length; i++) {
        sum += ((char*)tableHeader)[i];
    }
 
    return sum == 0;
}

ACPISDTHeader* acpi_find_table(uint32_t rsdt_addr, uint32_t sdt_count, const char* signature) {
    uint32_t* rsdt_end = (uint32_t*)(rsdt_addr + sizeof(ACPISDTHeader));

    map_pages_overlapping(
            get_kernel_page_directory(),
            rsdt_addr,
            rsdt_addr,
            sdt_count * sizeof(uint32_t),
            PAGE_PRESENT
    );

    qemu_log("RSDT start: %x", rsdt_addr);
    qemu_log("RSDT end: %x", (size_t)rsdt_end);
    qemu_log("RSDT size: %d", sizeof(ACPISDTHeader) + (sdt_count * sizeof(uint32_t)));

    for(uint32_t i = 0; i < sdt_count; i++) {
        ACPISDTHeader* entry = (ACPISDTHeader*)(rsdt_end[i]);

        if(memcmp(entry->Signature, signature, 4) == 0) {
            return entry;
        }
    }

    return 0;
}

void acpi_scan_all_tables(uint32_t rsdt_addr) {
    ACPISDTHeader* rsdt = (ACPISDTHeader*)rsdt_addr;

    map_pages_overlapping(
        get_kernel_page_directory(),
        rsdt_addr,
		rsdt_addr,
        sizeof(ACPISDTHeader),
        PAGE_PRESENT
    );
    
    size_t rsdt_length = rsdt->Length;
    uint32_t sdt_count = (rsdt_length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);

    map_pages_overlapping(
        get_kernel_page_directory(),
        rsdt_addr,
		rsdt_addr,
        rsdt->Length,
        PAGE_PRESENT
    );

    qemu_log("RSDT length: %d (sizeof(ACPISDTHeader) == %d)", rsdt->Length, sizeof(ACPISDTHeader));

    uint32_t* rsdt_end = (uint32_t*)(rsdt_addr + sizeof(ACPISDTHeader));

    uint32_t* addresses = kcalloc(sdt_count, sizeof(uint32_t));

    memcpy(addresses, rsdt_end, sdt_count * sizeof(uint32_t));

    qemu_printf("Addresses: [");
    
    for(register int i = 0; i < sdt_count; i++) {
        qemu_printf("%x, ", addresses[i]);
    }

    qemu_printf("]\n");

    qemu_log("RSDT start: %x", rsdt_addr);
    qemu_log("RSDT end: %x", (size_t)rsdt_end);

    qemu_log("SDT COUNT: %u", sdt_count);

    for(uint32_t i = 0; i < sdt_count; i++) {
        qemu_log("Mapping %x", addresses[i]);
        map_pages_overlapping(
            get_kernel_page_directory(),
            addresses[i],
            addresses[i],
            PAGE_SIZE,
            PAGE_PRESENT
        );

        ACPISDTHeader* entry = (ACPISDTHeader*)(addresses[i]);

		tty_printf("[%d/%d] [%x] Found table: %.4s (len: %d)\n", i, sdt_count, entry, entry->Signature, entry->Length);
		qemu_log("[%x] Found table: %.4s (len: %d)", (size_t)entry, entry->Signature, entry->Length);

        qemu_log("Unmapping %x", addresses[i]);
    }

    kfree(addresses);
}


void acpi_find_facp(size_t rsdt_addr) {
	qemu_log("FACP at P%x", rsdt_addr);

    map_pages_overlapping(
        get_kernel_page_directory(),
        rsdt_addr,
		rsdt_addr,
        PAGE_SIZE * 2,
        PAGE_PRESENT
    );

    ACPISDTHeader* rsdt = (ACPISDTHeader*)rsdt_addr;

//	new_qemu_printf("OEM: %.11s\n", rsdt->OEMID);
//	qemu_log("Length: %d", rsdt->Length);
//	new_qemu_printf("OEMTableID: %.8s\n", rsdt->OEMTableID);

    bool check = acpi_checksum_sdt(rsdt);

    qemu_log("Checksum: %s", check ? "PASS" : "FAIL");

    if(!check) {
        qemu_log("INVALID RSDT TABLE!");
        return;
    }

    qemu_log("OEMID: %s", rsdt->OEMID);
    qemu_log("Length: %d bytes", rsdt->Length);

    uint32_t sdt_count = (rsdt->Length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);

    qemu_log("SDTs available: %d", sdt_count);

    // Find FACP here

    ACPISDTHeader* pre_fadt = acpi_find_table((uint32_t) rsdt_addr, sdt_count, "FACP");

    if(!pre_fadt) {
        qemu_log("FADT not found...");
        return;
    } else {
      qemu_log("FADT found at: %x (With length: %d bytes) (struct is: %d bytes long)", pre_fadt, pre_fadt->Length, sizeof(struct FADT));
    qemu_log("Revision: %x", pre_fadt->Revision);
    }

    struct FADT* fadt = (struct FADT*)(pre_fadt + 1);  // It skips SDT header
  //
    //hexview_advanced(fadt, pre_fadt->Length - 36, 24, false, new_qemu_printf);

//    map_single_page(get_kernel_page_directory(), (virtual_addr_t) fadt, (virtual_addr_t) fadt, PAGE_WRITEABLE);

    qemu_log("DSDT: %x", fadt->Dsdt);
    qemu_log("SMI port: %x", fadt->SMI_CommandPort);
    qemu_log("Enable Command: %x", fadt->AcpiEnable);
    qemu_log("FirmwareControl: %x", fadt->FirmwareCtrl);
    qemu_log("Century: %x", fadt->Century);
    qemu_log("ResetRegAddress(L): %x", fadt->ResetReg.AddressLow);
    qemu_log("ResetRegAddress(H): %x", fadt->ResetReg.AddressHigh);
    qemu_log("ResetReg AddressSpace: %d", fadt->ResetReg.AddressSpace);
    qemu_log("ResetValue: %x", fadt->ResetValue);
    qemu_log("TODO: Write 'Enable Command' to 'SMI Port' using `outb()` func");

    if(fadt->Century) {
        century_register = fadt->Century;
    }

    //uint32_t reset_reg = fadt->ResetReg.AddressHigh;
    //map_single_page(get_kernel_page_directory(), reset_reg, reset_reg, PAGE_WRITEABLE);
    //*(uint8_t*)reset_reg = fadt->ResetValue;

    qemu_log("Found FADT!");

//    unmap_single_page(get_kernel_page_directory(), (virtual_addr_t) fadt);
    unmap_pages_overlapping(get_kernel_page_directory(), (virtual_addr_t) rsdt_addr, PAGE_SIZE * 2);
}

void acpi_find_apic(size_t rsdt_addr, size_t *lapic_addr) {
	size_t start = rsdt_addr & ~0xfff;
	size_t end = ALIGN(rsdt_addr + PAGE_SIZE, PAGE_SIZE);

    map_pages(
        get_kernel_page_directory(),
        start,
        start,
        end - start,
        PAGE_PRESENT
    );

	qemu_log("!!! Should be: %x - %x", start, end);
	qemu_log("!!! Mapped memory range: %x - %x", rsdt_addr, rsdt_addr + (PAGE_SIZE));
    ACPISDTHeader* rsdt = (ACPISDTHeader*)rsdt_addr;

    bool check = acpi_checksum_sdt(rsdt);

    qemu_log("Checksum: %s", check ? "PASS" : "FAIL");

    if(!check) {
        qemu_log("INVALID RSDT TABLE!");
        return;
    }

    qemu_log("OEMID: %s", rsdt->OEMID);
    qemu_log("Length: %d entries", rsdt->Length);

    uint32_t sdt_count = (rsdt->Length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);

    qemu_log("SDTs available: %d", sdt_count);

    // Find APIC here

    ACPISDTHeader* apic = acpi_find_table(rsdt_addr, sdt_count, "APIC");

    if(!apic) {
        qemu_log("APIC not found...");
        return;
    }

    qemu_log("Found APIC!");

    size_t table_end = (size_t)apic + sizeof(ACPISDTHeader);

	qemu_log("Table end at: %x", table_end);

    struct APIC_Base_Table* apic_base = (struct APIC_Base_Table*)table_end;

    qemu_log("LAPIC at: %x", apic_base->lapic_addr);
    qemu_log("Flags: %x", apic_base->flags);

    *lapic_addr = apic_base->lapic_addr;

    size_t base_table_end = table_end + sizeof(struct APIC_Base_Table);

    for(size_t i = 0; i < sdt_count; i++) {
        struct APIC_Entry* entry = (struct APIC_Entry*)base_table_end;

        qemu_log("[%x] Type: %d", entry, entry->type);

        switch(entry->type) {
            case APIC_PLAPIC: {
                qemu_log("PLAPIC!");

                qemu_log("- Processor ID: %d", entry->entry.plapic.processor_id);
                qemu_log("- Flags: %x", entry->entry.plapic.flags);
                qemu_log("- APIC ID: %x", entry->entry.plapic.apic_id);

				system_processors_found++;
                
                break;
            }

            case APIC_IOAPIC: {
                qemu_log("IOAPIC!");

                qemu_log("- ID: %d", entry->entry.ioapic.id);
                qemu_log("- IO APIC Address: %x", entry->entry.ioapic.io_apic_address);
                qemu_log("- Global System Interrupt Base: %x", entry->entry.ioapic.global_system_interrupt_base);
                
                break;
            }

            case APIC_IOAPIC_ISO: {
                qemu_log("IOAPIC Interrupt source override!");

                qemu_log("- Bus Source: %d", entry->entry.ioapic_iso.bus_source);
                qemu_log("- IRQ Source: %d", entry->entry.ioapic_iso.irq_source);
                qemu_log("- Global System Interrupt: %x", entry->entry.ioapic_iso.global_system_interrupt);
                qemu_log("- Flags: %x", entry->entry.ioapic_iso.flags);
                
                break;
            }

            case APIC_IOAPIC_NMI: {
                qemu_log("IOAPIC NMI!");

                qemu_log("- Source: %x", entry->entry.ioapic_nmi.source);
                qemu_log("- Global System Interrupt: %x", entry->entry.ioapic_nmi.global_system_interrupt);
                qemu_log("- Flags: %x", entry->entry.ioapic_nmi.flags);
                
                break;
            }

            case APIC_LAPIC_NMI: {
                qemu_log("LAPIC NMI!");

                qemu_log("- Processor ID: %d", entry->entry.lapic_nmi.processor_id);
                qemu_log("- LINT: %d", entry->entry.lapic_nmi.lint);
                qemu_log("- Flags: %x", entry->entry.lapic_nmi.flags);
                
                break;
            }

            default: {
                qemu_log("Unknown type! [%d]", entry->type);

                goto apic_detect_end;
            }
        }

        base_table_end += entry->record_length;
    }

    apic_detect_end:

    // unmap_single_page(get_kernel_page_directory(), rsdt_addr);
}
