#pragma once

#include <common.h>

/**
 * @brief Отправка одного байта в порт
 *
 * @param port - порт
 * @param val - данные
 */
 SAYORI_INLINE void outb(uint16_t port, uint8_t val) {
	__asm__ volatile("outb %b0, %w1" : : "a"(val), "Nd"(port));
}

/**
 * @brief Получение одного байта из порта
 *
 * @param port - порт
 * @return Байт из порта
 */
SAYORI_INLINE uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;

}

/**
 * @brief Запись 32-битного слова в порт
 *
 * @param port - порт
 * @param val - число
 */
SAYORI_INLINE void outl(uint16_t port, uint32_t val) {
	__asm__ volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

/**
 * @brief Чтение 32-битного слова
 *
 * @param port - порт
 * @return Слово из порта
 */
SAYORI_INLINE uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile( "inl %1, %0" : "=a"(ret) : "Nd"(port) );
	return ret;
}

/**
 * @brief Чтение 16-битного слова из порта
 *
 * @param port - порт
 * @return Слово из порта
 */
SAYORI_INLINE uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

/**
 * @brief Запись 16-битного слова в порт
 *
 * @param port - порт
 * @param data - данные
 */
SAYORI_INLINE void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %1, %0" :: "Nd" (port), "a" (data));
}