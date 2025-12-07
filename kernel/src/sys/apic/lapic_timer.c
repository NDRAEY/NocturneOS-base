#include "arch/x86/mem/paging_common.h"
#include "arch/x86/pit.h"
#include "arch/x86/ports.h"
#include "sys/acpi.h"
#include <io/logging.h>
#include "sys/apic.h"

extern size_t lapic_addr;

void lapic_init() {
    // Did by code example from https://wiki.osdev.org/APIC_timer

    qemu_log("Initializing LAPIC timer");

    apic_write(APIC_REG_TMRDIV, 0x03);
    apic_write(APIC_REG_TMRINITCNT, 0xFFFFFFFF);

    __asm__ volatile("sti");

    // USING PIT.
    sleep_ms(20);

    __asm__ volatile("cli");

    apic_write(APIC_REG_LVT_TMR, 0x1000);

    uint32_t ticks_in_20_ms = 0xFFFFFFFF - apic_read(APIC_REG_TMRCURRCNT);
    uint32_t ticks_in_1_ms = ticks_in_20_ms / 20;
    uint32_t ticks_in_1_ns = ticks_in_1_ms / 1000;

    qemu_log("LAPIC ticks %d times in 1 PIT millisecond", ticks_in_1_ms);

    // Disable PIT by setting one-shot mode and never using it again.
    outb(0x43, (0b00 << 6) | (0b11 << 4) | (1 << 1));

    outb(0x40, 0xff);
    outb(0x40, 0xff);

    // 32 is IRQ0 - PIT timer. 17th bit is periodic mode flag for timer.
    apic_write(APIC_REG_LVT_TMR, 32 | (1 << 17));
    apic_write(APIC_REG_TMRDIV, 0x03);
    apic_write(APIC_REG_TMRINITCNT, ticks_in_1_ms);
}