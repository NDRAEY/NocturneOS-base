#include "arch/x86/mem/paging_common.h"
#include "arch/x86/pit.h"
#include "arch/x86/ports.h"
#include "sys/acpi.h"
#include <io/logging.h>
#include "sys/apic.h"

extern size_t timer_frequency;

// Measure LAPIC Timer tick count for given `milliseconds`.
size_t recalibrate_lapic_timer(size_t divisor, size_t milliseconds) {
    apic_write(APIC_REG_TMRDIV, divisor);
    apic_write(APIC_REG_TMRINITCNT, 0xFFFFFFFF);

    __asm__ volatile("sti");

    // USING PIT.
    sleep_ms(milliseconds);

    __asm__ volatile("cli");

    // Stop timer (Set masking bit).
    apic_write(APIC_REG_LVT_TMR, 0x1000);

    return 0xFFFFFFFF - apic_read(APIC_REG_TMRCURRCNT);
}

const size_t LAPIC_TIMER_DIVISOR = 0x03;

void lapic_init() {
    // Did by code example from https://wiki.osdev.org/APIC_timer

    qemu_log("Initializing LAPIC timer");

    // Calculate average tick count (6 times).
    uint32_t ticks_in_1_ms = recalibrate_lapic_timer(LAPIC_TIMER_DIVISOR, 5) / 5;

    for(register size_t i = 0; i < 5; i++) {
        ticks_in_1_ms += recalibrate_lapic_timer(LAPIC_TIMER_DIVISOR, 5) / 5;

        ticks_in_1_ms /= 2;
    }

    qemu_log("LAPIC ticks %d times in 1 PIT millisecond", ticks_in_1_ms);

    // Disable PIT by setting one-shot mode and never using it again.
    outb(0x43, (0b00 << 6) | (0b11 << 4) | (1 << 1));

    outb(0x40, 0xff);
    outb(0x40, 0xff);

    // 32 is IRQ0 - PIT timer. 17th bit is periodic mode flag for timer.
    apic_write(APIC_REG_LVT_TMR, 32 | (1 << 17));
    apic_write(APIC_REG_TMRDIV, LAPIC_TIMER_DIVISOR);
    apic_write(APIC_REG_TMRINITCNT, ticks_in_1_ms);
}