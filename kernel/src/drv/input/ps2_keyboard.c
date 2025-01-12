/**
 * @file drv/input/keyboard.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Драйвер клавиатуры
 * @version 0.3.5
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */
extern void tty_backspace();

#include <lib/string.h>
#include <io/ports.h>
#include <sys/trigger.h>
#include "drv/input/keyboard.h"
#include "sys/sync.h"
#include "sys/timer.h"
#include "io/tty.h"
#include "drv/psf.h"
#include "sys/isr.h"
#include "drv/ps2.h"
#include <lib/keymap.h>

/**
 * @brief Обработчик клавиатуры
 */
void keyboardHandler(registers_t regs){
    uint32_t kbdstatus = inb(PS2_STATE_REG);

    if (kbdstatus & 0x01) {
        keyboard_buffer_put(ps2_read());
    }
}

/**
 * @brief Выполняет инициализацию клавиатуры
 */
void ps2_keyboard_init() {
    uint8_t stat;

    // ps2_in_wait_until_empty();

    // outb(PS2_DATA_PORT, 0xf4);
    
    ps2_write(0xf4);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Enable fail");
        return;
    }

    // ps2_in_wait_until_empty();

    // outb(PS2_DATA_PORT, 0xf0);

    ps2_write(0xf0);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Scancode set fail");
        return;
    }

    // ps2_in_wait_until_empty();

    // outb(PS2_DATA_PORT, 0);
    ps2_write(0);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Zero fail");
        return;
    }

    size_t scancode = ps2_read() & 0b11;

    qemu_note("SCANCODE SET: %d", scancode);

    // ps2_in_wait_until_empty();

    // outb(PS2_DATA_PORT, 0xf3);
    ps2_write(0xf3);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Repeat fail");
        return;
    }

    // ps2_in_wait_until_empty();

    // outb(PS2_DATA_PORT, 0);
    ps2_write(0);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Zero fail (phase 2)");
        return;
    }

    uint8_t conf = ps2_read_configuration_byte();

    ps2_write_configuration_byte(conf | 0b1000001);
}

void ps2_keyboard_install_irq() {
    register_interrupt_handler(IRQ1, &keyboardHandler);
    qemu_log("Keyboard installed");
}
