#include <common.h>
#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>
#include "arch/x86_64/idt64.h"
#include "arch/x86/pit.h"
#include "arch/x86/isr.h"
#include "arch/x86_64/registers64.h"
#include <multiboot.h>

void __attribute__((noreturn)) arch_init(const multiboot_header_t *mboot) {
    qemu_log("Hello, world! (MBOOT = %x)", (uint32_t)(size_t)mboot);

    init_idt();
    isr_init();

    init_timer(1000);
    
    while(1) {
        qemu_log("Timer ticks: %d", getTicks());
        sleep_ms(1000);
    }

    while(1)
        ;
}
