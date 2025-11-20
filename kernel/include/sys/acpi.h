#pragma once

#include "common.h"
#include "sys/rsdp.h"
#include "sys/rsdt.h"

extern uint32_t system_processors_found;

RSDPDescriptor* acpi_rsdp_find();
void acpi_find_facp(size_t rsdt_addr);
void acpi_find_apic(size_t rsdt_addr, size_t *lapic_addr);
void acpi_scan_all_tables(uint32_t rsdt_addr);
ACPISDTHeader* acpi_find_table(uint32_t rsdt_addr, uint32_t sdt_count, const char* signature);