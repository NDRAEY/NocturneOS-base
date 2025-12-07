#pragma once

#include <common.h>

#define APIC_REG_APICID	        0x020
#define APIC_REG_APICVER	    0x030
#define APIC_REG_TASKPRIOR	    0x080
#define APIC_REG_EOI	        0x0B0
#define APIC_REG_LDR            0x0D0
#define APIC_REG_DFR	        0x0E0
#define APIC_REG_SPURIOUS       0x0F0

#define APIC_REG_ESR            0x280
#define APIC_REG_ICRL           0x300
#define APIC_REG_ICRH           0x310
#define APIC_REG_LVT_TMR        0x320
#define APIC_REG_LVT_PERF       0x340
#define APIC_REG_LVT_LINT0      0x350
#define APIC_REG_LVT_LINT1      0x360
#define APIC_REG_LVT_ERR        0x370
#define APIC_REG_TMRINITCNT     0x380
#define APIC_REG_TMRCURRCNT     0x390
#define APIC_REG_TMRDIV         0x3E0
#define APIC_REG_LAST           0x38F

extern volatile bool __using_apic;
extern volatile size_t lapic_addr;

enum APIC_Type {
    APIC_PLAPIC = 0,
    APIC_IOAPIC = 1,
    APIC_IOAPIC_ISO = 2,
    APIC_IOAPIC_NMI = 3,
    APIC_LAPIC_NMI = 4,
    APIC_LAPIC_OVERRIDE = 5,
    APIC_PLx2APIC = 9
};

struct APIC_Base_Table {
    uint32_t lapic_addr;
    uint32_t flags;
} __attribute__((packed));

// Entry Type 0: Processor Local APIC
struct APIC_PLAPIC {
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

// Entry Type 1: I/O APIC
struct APIC_IOAPIC {
    uint8_t id;
    uint8_t reserved;  // Should be 0
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

// Entry Type 2: IO/APIC Interrupt source override
struct APIC_IOAPIC_ISO {
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed));

// Entry Type 3: IO/APIC NMI Source
struct APIC_IOAPIC_NMI {
    uint8_t source;
    uint8_t reserved;  // Should be 0
    uint16_t flags;
    uint32_t global_system_interrupt;
} __attribute__((packed));

// Entry Type 4: Local APIC NMI Source
struct APIC_LAPIC_NMI {
    uint8_t processor_id;  // 0xFF = all processors
    uint16_t flags;
    uint8_t lint;  // Local Interrupt? (0 or 1)
} __attribute__((packed));

// Entry Type 5: Local APIC Address Override
struct APIC_LAPIC_OVERRIDE {
    uint16_t reserved;
    uint32_t lapic_phys_addr_low;
    uint32_t lapic_phys_addr_high;

    // On 64-bit systems
    // lapic_phys_addr = (lapic_phys_addr_low << 32) | lapic_phys_addr_high;
} __attribute__((packed));

// Entry Type 9: Processor Local x2APIC
struct APIC_PLx2APIC {
    uint16_t reserved;
    uint32_t processor_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed));

struct APIC_Entry {
    uint8_t type;
    uint8_t record_length;

    union {
        struct APIC_PLAPIC plapic;
        struct APIC_IOAPIC ioapic;
        struct APIC_IOAPIC_ISO ioapic_iso;
        struct APIC_IOAPIC_NMI ioapic_nmi;
        struct APIC_LAPIC_NMI lapic_nmi;
        struct APIC_LAPIC_OVERRIDE lapic_override;
        struct APIC_PLx2APIC plx2apic;
    } entry;
} __attribute__((packed));

void apic_init();

SAYORI_INLINE bool apic_is_enabled() {
    return __using_apic;
}

uint32_t apic_write(uint32_t reg, uint32_t value);
uint32_t apic_read(uint32_t reg);