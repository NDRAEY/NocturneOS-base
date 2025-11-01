#include <common.h>
#include <arch/x86/ports.h>
#include <arch/x86/serial_port.h>

void arch_init() {
    char* string = "Finally x86_64!\n";

    while(*string) {
        outb(PORT_COM1, *string);

        string++;
    }

    while(1)
        ;
}
