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
 * @brief [PCI] Категория устройств
 *
 */
static struct {
    uint8_t klass, subclass;
    const char *name;
} pci_device_type_strings[] = {
    {0x00, 0x00, "Неизвестное устройство"},
    {0x00, 0x01, "VGA-совместимое устройство"},
    {0x01, 0x00, "Контроллер шины SCSI"},
    {0x01, 0x01, "IDE-контроллер"},
    {0x01, 0x02, "Контроллер гибких дисков"},
    {0x01, 0x03, "Контроллер шины IPI"},
    {0x01, 0x04, "RAID-контроллер"},
    {0x01, 0x05, "Контроллер АТА"},
    {0x01, 0x06, "SATA-контроллер"},
    {0x01, 0x07, "Последовательный подключенный контроллер SCSI"},
    {0x01, 0x80, "Другой контроллер запоминающих устройств"},
    {0x02, 0x00, "Ethernet-контроллер"},
    {0x02, 0x01, "Контроллер Token Ring"},
    {0x02, 0x02, "FDDI-контроллер"},
    {0x02, 0x03, "Контроллер банкомата"},
    {0x02, 0x04, "ISDN-контроллер"},
    {0x02, 0x05, "Контроллер WorldFip"},
    {0x02, 0x06, "PICMG 2.14 Мультивычисления"},
    {0x02, 0x80, "Другой сетевой контроллер"},
    {0x03, 0x00, "VGA-совместимый контроллер"},
    {0x03, 0x01, "XGA-контроллер"},
    {0x03, 0x02, "3D контроллер"},
    {0x03, 0x80, "Другой контроллер дисплея"},
    {0x04, 0x00, "Видео-устройство"},
    {0x04, 0x01, "Аудио-устройство"},
    {0x04, 0x02, "Компьютерное телефонное устройство"},
    {0x04, 0x03, "Аудио-устройство (4.3)"},
    {0x04, 0x80, "Другое мультимедийное устройство"},
    {0x05, 0x00, "Контроллер оперативной памяти"},
    {0x05, 0x01, "Флэш-контроллер"},
    {0x05, 0x80, "Другой контроллер памяти"},
    {0x06, 0x00, "Хост-мост"},
    {0x06, 0x01, "ISA мост"},
    {0x06, 0x02, "EISA мост"},
    {0x06, 0x03, "MCA мост"},
    {0x06, 0x04, "PCI-to-PCI мост"},
    {0x06, 0x05, "PCMCIA мост"},
    {0x06, 0x06, "NuBus мост"},
    {0x06, 0x07, "CardBus мост"},
    {0x06, 0x08, "RACEWay мост"},
    {0x06, 0x09, "PCI-to-PCI мост (Полупрозрачный)"},
    {0x06, 0x0A, "Хост-мост InfiniBand-PCI"},
    {0x06, 0x80, "Другое устройство моста"},
    {0x07, 0x00, "Последовательный контроллер"},
    {0x07, 0x01, "Параллельный порт"},
    {0x07, 0x02, "Многопортовый последовательный контроллер"},
    {0x07, 0x03, "Универсальный модем"},
    {0x07, 0x04, "IEEE 488.1/2 (GPIB) контроллер"},
    {0x07, 0x05, "Интеллектуальная карточка"},
    {0x07, 0x80, "Другое устройство связи"},
    {0x08, 0x00, "Программируемый контроллер прерываний"},
    {0x08, 0x01, "Контроллер прямого доступа к памяти"},
    {0x08, 0x02, "Системный таймер"},
    {0x08, 0x03, "Часы реального времени"},
    {0x08, 0x04, "Универсальный контроллер PCI с возможностью горячей замены"},
    {0x08, 0x80, "Другая системная периферия"},
    {0x09, 0x00, "Контроллер клавиатуры"},
    {0x09, 0x01, "Цифровой преобразователь"},
    {0x09, 0x02, "Контроллер мыши"},
    {0x09, 0x03, "Контроллер сканера"},
    {0x09, 0x04, "Контроллер игрового порта"},
    {0x09, 0x80, "Другой контроллер ввода"},
    {0x0A, 0x00, "Универсальная док-станция"},
    {0x0A, 0x80, "Другая док-станция"},
    {0x0B, 0x00, "Процессор i386"},
    {0x0B, 0x01, "Процессор i486"},
    {0x0B, 0x02, "Процессор Pentium"},
    {0x0B, 0x10, "Процессор Alpha"},
    {0x0B, 0x20, "Процессор PowerPC"},
    {0x0B, 0x30, "Процессор MIPS"},
    {0x0B, 0x40, "Со-процессор"},
    {0x0C, 0x00, "Контроллер FireWire"},
    {0x0C, 0x01, "Контроллер ACCESS.bus"},
    {0x0C, 0x02, "SSA Контроллер"},
    {0x0C, 0x03, "USB Контроллер"},
    {0x0C, 0x04, "Волоконный канал"},
    {0x0C, 0x05, "SMBus"},
    {0x0C, 0x06, "InfiniBand"},
    {0x0C, 0x07, "Интерфейс IPMI SMIC"},
    {0x0C, 0x08, "Интерфейс SERCOS"},
    {0x0C, 0x09, "Интерфейс CANbus"},
    {0x0D, 0x00, "iRDA-совместимый контроллер"},
    {0x0D, 0x01, "Потребительский ИК-контроллер"},
    {0x0D, 0x10, "RF Контроллер"},
    {0x0D, 0x11, "Bluetooth Контроллер"},
    {0x0D, 0x12, "Broadband Контроллер"},
    {0x0D, 0x20, "802.11a (Wi-Fi) Ethernet-контроллер"},
    {0x0D, 0x21, "802.11b (Wi-Fi) Ethernet-контроллер"},
    {0x0D, 0x80, "Другой беспроводной контроллер"},
    {0x00, 0x00, nullptr} // Конец
};

/**
 * @brief [PCI] Поставщики устройств
 *
 */
static struct {
    uint16_t vendor;
    const char *name;
} pci_vendor_name_strings[] = {
    {0x8086, "Intel Corporation"},
    {0x10DE, "NVIDIA"},
    {0x0014, "Loongson Technology Corporation Limited"},
    {0x001C, "PEAK-System Technik GmbH"},
    {0x00B0, "Blue Origin, LLC"},
    {0x00BB, "Bloombase"},
    {0x0123, "General Dynamics Mission Systems, Inc."},
    {0x6688, "GUANGZHOU MAXSUN INFORMATION TECHNOLOGY CO., LTD."},
    {0x751A, "Tesla Inc."},
    {0x8080, "StoreSwift Technology Co., Ltd."},
    {0x1013, "Cirrus Logic, Inc."},
    {0x1014, "IBM"},
    {0x101E, "American Megatrends Incorporated"},
    {0x1028, "Dell Computer Corporation"},
    {0x102B, "Matrox Graphics Inc."},
    {0xA23B, "Silicon Integrated Systems"},
    {0x103C, "Hewlett Packard"},
    {0x1043, "Asustek Computer Inc."},
    {0x104C, "Texas Instruments"},
    {0x104D, "Sony Group Corporation"},
    {0x1054, "Hitachi, Ltd."},
    {0x1002, "[AMD] Advanced Micro Devices Inc."},
    {0x1022, "[AMD] Advanced Micro Devices Inc."},
    {0x10EC, "Realtek Semiconductor Corp."},
    {0x1039, "[SiS] Silicon Integrated Systems"},
    {0x0B05, "[ASUS] ASUSTek Computer, Inc."},
    {0x80EE, "[VirtualBox] InnoTek Systemberatung GmbH"},
    {0x1234, "[QEMU] Technical Corp"},
    {0x106B, "Apple Inc."},
    {0x1AF4, "Red Hat, Inc."},
    {0, nullptr}
};

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

/**
 * @brief [PCI] Получение классификации устройства
 *
 * @param klass Группа А
 * @param subclass Группа Б
 * @return Возращает классификацию устройства
 */
const char* pci_get_device_type(uint8_t klass, uint8_t subclass) {
    for (size_t i = 0; pci_device_type_strings[i].name != nullptr; i++) {
        if (pci_device_type_strings[i].klass == klass && pci_device_type_strings[i].subclass == subclass) {
            return pci_device_type_strings[i].name;
        }
    }        

    return nullptr;
}

/**
 * @brief [PCI] Получение названия поставщика
 *
 * @param vendor Поставщик
 * @return const char * Возращает имя поставщика
 */
const char *pci_get_vendor_name(uint16_t vendor) {
	for (int i = 0; pci_vendor_name_strings[i].name != nullptr; i++) {
		if(pci_vendor_name_strings[i].vendor == vendor) {
			return pci_vendor_name_strings[i].name;
        }
    }

	return "unknown";
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
