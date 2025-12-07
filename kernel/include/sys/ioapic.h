#pragma once

#include <common.h>

#define IOAPIC_REGSEL 0x00
#define IOAPIC_DATA   0x10

#define IOAPIC_REG_ID          0x00
#define IOAPIC_REG_VER         0x01
#define IOAPIC_REG_ARB         0x02
#define IOAPIC_REG_REDTBL_BASE 0x10

bool apic_find_ioapic();
uint32_t ioapic_read(uint32_t reg);
void ioapic_write(uint32_t reg, uint32_t value);
void ioapic_post_initialize();
    