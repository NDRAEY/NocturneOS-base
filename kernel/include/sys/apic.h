#pragma once

#include <common.h>

extern volatile bool __using_apic;

void apic_init();
uint32_t apic_write(uint32_t reg, uint32_t value);