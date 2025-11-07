#include <common.h>
#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>

void arch_init() {
    qemu_log("Hello, world!");

    while(1)
        ;
}
