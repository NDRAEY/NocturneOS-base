/**
 * @file io/ports.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Средства для работы с портами
 * @version 0.4.2
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include <stdarg.h>
#include <arch/x86/ports.h>
#include <lib/sprintf.h>
#include <lib/asprintf.h>
#include "io/serial_port.h"
#include "sys/scheduler.h"
#include "mem/vmm.h"

/**
 * @brief Чтение длинного слова через порт
 *
 * @param port - порт
 * @param buffer - данные
 * @param times - сколько данных прочесть
 */
// void insl(uint16_t port, uint32_t *buffer, uint32_t times) {
//     for (uint32_t index = 0; index < times; index++) {
//         buffer[index] = inl(port);
//     }
// }


/**
 * @brief Запись длинного слова через порт
 *
 * @param port - порт
 * @param buffer - данные
 * @param times - сколько данных отправить
 */
void outsl(uint16_t port, const uint32_t *buffer, int32_t times) {
    for (int32_t index = 0; index < times; index++) {
        outl(port, buffer[index]);
    }
}

void insw(uint16_t __port, void *__buf, unsigned long __n) {
	__asm__ volatile("cld; rep; insw"
			: "+D"(__buf), "+c"(__n)
			: "d"(__port));
}
 
void outsw(uint16_t __port, const void *__buf, unsigned long __n) {
	__asm__ volatile("cld; rep; outsw"
			: "+S"(__buf), "+c"(__n)
			: "d"(__port));
}

/**
 * @brief Небольшая задержка используя порт 128(0x80)
 */
void io_wait(void) {
    outb(0x80, 0);
}