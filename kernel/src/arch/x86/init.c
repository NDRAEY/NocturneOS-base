#include "arch/x86/serial_port.h"
#include "arch/x86/sse.h"
#include "multiboot.h"

extern void fpu_save();

void __attribute__((noreturn)) kmain(const multiboot_header_t *mboot, uint32_t initial_esp);

size_t __init_esp;

void __attribute((noreturn)) arch_init(const multiboot_header_t *mboot, uint32_t initial_esp) {
    if (sse_check()) {
        fpu_save();
    }

    __init_esp = initial_esp;
    
    __com_init(PORT_COM1);

    kmain(mboot, initial_esp);
}
