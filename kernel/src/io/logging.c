#include <common.h>

#ifdef NOCTURNE_X86
#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>

// Check if character received.
#define is_signal_received(port) (inb(port + 5) & 1)

#endif

#include <io/logging.h>
#include <mem/vmm.h>
#include <lib/asprintf.h>
#include <sys/scheduler.h>
#include <io/ports.h>

void (*default_qemu_printf)(const char *text, ...) = qemu_printf;

void switch_qemu_logging() {
    default_qemu_printf = new_qemu_printf;
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

    scheduler_mode(false);  // Stop scheduler

    __com_pre_formatString(PORT_COM1, text, args);

    scheduler_mode(true);  // Start scheduler
    
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
