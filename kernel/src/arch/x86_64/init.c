#include <common.h>
#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>
#include <multiboot.h>

void __attribute__((noreturn)) arch_init(const multiboot_header_t *mboot) {
    qemu_log("Hello, world! (MBOOT = %x)", (uint32_t)(size_t)mboot);
    
    while(1)
        ;
}
