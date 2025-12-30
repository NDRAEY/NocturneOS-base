/**
 * @defgroup pci Драйвер PCI (Peripheral Component Interconnect)
 * @file drv/pci.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), Арен Елчинян (SynapseOS), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Драйвер PCI (Peripheral Component Interconnect)
 * @version 0.4.3
 * @date 2023-01-14
 * @copyright Copyright SayoriOS Team (c) 2022-2026
 */

#include <generated/pci.h>

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