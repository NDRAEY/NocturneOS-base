/**
 * @file io/ports.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Средства для работы с портами
 * @version 0.4.2
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include <arch/x86/ports.h>
#include "io/serial_port.h"
#include "sys/scheduler.h"
#include "mem/vmm.h"

/**
 * @brief Небольшая задержка используя порт 128(0x80)
 */
void io_wait(void) {
    outb(0x80, 0);
}