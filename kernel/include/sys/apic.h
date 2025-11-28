#pragma once

#include <common.h>

void apic_init();
uint32_t apic_write(uint32_t reg, uint32_t value);