#include <common.h>

#ifdef NOCTURNE_X86
#include <arch/x86/ports.h>

// Check if character received.
#define is_signal_received(port) (inb(port + 5) & 1)

#endif

#include <io/logging.h>
#include <mem/vmm.h>
#include <lib/asprintf.h>
#include <sys/scheduler.h>
#include <io/serial_port.h>
#include <io/ports.h>

void (*default_qemu_printf)(const char *text, ...) = qemu_printf;

void switch_qemu_logging() {
    default_qemu_printf = new_qemu_printf;
}

/**
 * @brief Проверка занятости порта
 *
 * @return int32_t - состояние
 */
int32_t is_transmit_empty(uint16_t port) {
    return inb(port + 5) & 0x20;
}

// Read 1 byte (char) from port.
uint8_t serial_readchar(uint16_t port) {
    while (is_signal_received(port) == 0)
        ;
    return inb(port);
}

// Read 1 byte (char) from port.
int8_t serial_readchar_timeout(uint16_t port,size_t timeout, bool Alert) {
    size_t to = 0;
    while (is_signal_received(port) == 0){
        to++;
        //qemu_warn("TIMEOUT: %d",to);
        if (to >= timeout){
            if (Alert) {
                qemu_warn("TIMEOUT: %d",to);
            }
            return -1;
        }
    }
    return inb(port);
}

/**
 * @brief Вывод QEMU через COM1 информации
 *
 * @param text Форматная строка
 * @param ... Дополнительные параметры
 */
 void qemu_printf(const char *text, ...) {
    va_list args;
    va_start(args, text);

    if (__com_getInit(1)) {
        scheduler_mode(false);  // Stop scheduler

        __com_pre_formatString(PORT_COM1, text, args);

        scheduler_mode(true);  // Start scheduler
    }
    
    va_end(args);
}

void new_qemu_printf(const char *format, ...) {
    if (!__com_getInit(1))
        return;

    va_list args;
    va_start(args, format);

    char* container;

    vasprintf(&container, format, args);

    va_end(args);

    scheduler_mode(false);  // Stop scheduler
    __com_writeString(PORT_COM1, container);
    scheduler_mode(true);  // Start scheduler

    kfree(container);
}
