/**
 * @defgroup pci Драйвер PCI (Peripheral Component Interconnect)
 * @file drv/pci.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), Арен Елчинян (SynapseOS), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Драйвер PCI (Peripheral Component Interconnect)
 * @version 0.3.5
 * @date 2023-01-14
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include <lib/stdio.h>
#include <io/ports.h>
#include <drv/pci.h>
#include "io/tty.h"
#include "../lib/libvector/include/vector.h"
#include "mem/vmm.h"

vector_t* pci_device_list = 0;

/**
 * @brief [PCI] Поставщики устройств
 *
 */
// static struct {
//     uint16_t vendor;
//     const char *name;
// } pci_vendor_name_strings[] = {
//     {0x8086, "Intel Corporation"},
//     {0x10DE, "NVIDIA"},
//     {0x0014, "Loongson Technology Corporation Limited"},
//     {0x001C, "PEAK-System Technik GmbH"},
//     {0x00B0, "Blue Origin, LLC"},
//     {0x00BB, "Bloombase"},
//     {0x0123, "General Dynamics Mission Systems, Inc."},
//     {0x6688, "GUANGZHOU MAXSUN INFORMATION TECHNOLOGY CO., LTD."},
//     {0x751A, "Tesla Inc."},
//     {0x8080, "StoreSwift Technology Co., Ltd."},
//     {0x1013, "Cirrus Logic, Inc."},
//     {0x1014, "IBM"},
//     {0x101E, "American Megatrends Incorporated"},
//     {0x1028, "Dell Computer Corporation"},
//     {0x102B, "Matrox Graphics Inc."},
//     {0xA23B, "Silicon Integrated Systems"},
//     {0x103C, "Hewlett Packard"},
//     {0x1043, "Asustek Computer Inc."},
//     {0x104C, "Texas Instruments"},
//     {0x104D, "Sony Group Corporation"},
//     {0x1054, "Hitachi, Ltd."},
//     {0x1002, "[AMD] Advanced Micro Devices Inc."},
//     {0x1022, "[AMD] Advanced Micro Devices Inc."},
//     {0x10EC, "Realtek Semiconductor Corp."},
//     {0x1039, "[SiS] Silicon Integrated Systems"},
//     {0x0B05, "[ASUS] ASUSTek Computer, Inc."},
//     {0x80EE, "[VirtualBox] InnoTek Systemberatung GmbH"},
//     {0x1234, "[QEMU] Technical Corp"},
//     {0x106B, "Apple Inc."},
//     {0x1AF4, "Red Hat, Inc."},
//     {0, nullptr}
// };

SAYORI_INLINE uint8_t pci_get_class(uint8_t bus, uint8_t slot, uint8_t function) {
    return (uint8_t)(pci_read32(bus, slot, function, 0xB) & 0xff);
}

SAYORI_INLINE uint8_t pci_get_subclass(uint8_t bus, uint8_t slot, uint8_t function) {
    return (uint8_t)(pci_read32(bus, slot, function, 0xA) & 0xff);
}

SAYORI_INLINE uint8_t pci_get_hdr_type(uint8_t bus, uint8_t slot, uint8_t function) {
    return (uint8_t)(pci_read32(bus, slot, function, 0xE) & 0xff);
}

SAYORI_INLINE uint16_t pci_get_vendor(uint8_t bus, uint8_t slot, uint8_t function) {
    return (uint16_t)(pci_read32(bus, slot, function, 0) & 0xffff);
}

uint16_t pci_get_device(uint8_t bus, uint8_t slot, uint8_t function) {
    return (uint16_t)(pci_read32(bus, slot, function, 0x2) & 0xffff);
}

uint32_t pci_get_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_number, pci_bar_type_t* bar_type_out) {
    size_t bar_offset = 0x10 + (bar_number * 0x4);
    size_t data = pci_read32(bus, slot, func, bar_offset);
    
    pci_bar_type_t bar_type = (data & 0x1);

    if(bar_type_out != NULL) {
        *bar_type_out = bar_type;
    }

    if(bar_type == MEMORY_BAR) {
        return data & ~0xF;   // First 4 bits
    } else {
        return data & ~0b11;  // First 2 bits
    }

    return 0;
}
