#include <common.h>
#include <arch/x86/ports.h>

/**
 * @brief Перезагрузка устройства
 */
void reboot() {
    uint8_t good = 0x02;

    while (good & 0x02) {
        good = inb(0x64);
    }

    outb(0x64, 0xFE);
    
    __asm__ volatile("hlt");
}

/**
 * @brief Выключение устройства
 */
void shutdown() {
    outw(0xB004, 0x2000);
    outw(0x604,  0x2000);
    outw(0x4004, 0x3400);
}