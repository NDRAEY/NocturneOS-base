#include "lib/sprintf.h"
#include <common.h>

#ifdef NOCTURNE_X86
#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>

// Check if character received.
#define is_signal_received(port) (inb(port + 5) & 1)

#endif

#ifdef NOCTURNE_X86_64

#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>

// Check if character received.
#define is_signal_received(port) (inb(port + 5) & 1)

#endif

#include <io/logging.h>
#include <mem/vmm.h>
#include <lib/asprintf.h>
#include <sys/scheduler/scheduler.h>
#include <io/ports.h>

void (*default_qemu_printf)(const char *text, ...) = qemu_printf;

void qemu_printchar_wrapper(char chr, SAYORI_UNUSED void* argument) {
    __com_writeChar(PORT_COM1, chr);
}

void qemu_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    #ifdef NOCTURNE_FEATURE_MULTITASKING
    scheduler_mode(false);  // Stop scheduler
    vfctprintf(qemu_printchar_wrapper, 0, format, args);
    scheduler_mode(true);  // Start scheduler
    #else
    vfctprintf(qemu_printchar_wrapper, 0, format, args);
    #endif

    va_end(args);
}
