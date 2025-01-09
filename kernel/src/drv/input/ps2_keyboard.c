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
#include <drv/input/keymap.h>

volatile int     lastKey = 0;            ///< Последний индекс клавиши
volatile uint32_t keyboard_states = 0;

static const char keyboard_layout[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0, ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0
};

// Определяем таблицу символов для клавиш при зажатой клавише Shift
static const char shifted_keyboard_layout[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,   'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0, ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0
};

volatile char kmode = 0;
volatile char* curbuf = 0;
volatile uint32_t chartyped = 0;

uint8_t getPressReleaseKeyboard() {
    return lastKey & 0x80; // if true -> Released / else - Pressed
}

int getCharRaw() {
    return lastKey;
}

int getIntKeyboardWait(){
    bool kmutex = false;
    mutex_get(&kmutex, true);

    while(lastKey==0 || (lastKey & 0x80)) {}

    mutex_release(&kmutex);
    return lastKey;
}

void kbd_add_char(char *buf, char* key) {
	if(kmode == 1 && curbuf != 0) {
		if (!(lastKey == 0x1C || lastKey == 0x0E)) {
			strcat(buf, key);
			chartyped++;
		}
		
		if(lastKey == 0x0E) { // BACKSPACE
            if(chartyped > 0) {
				tty_backspace();
				chartyped--;
				buf[chartyped] = 0;
			}
        }
	} else if(kmode == 2) {
		curbuf = key;
	}
}

void gets(char *buffer) { // TODO: Backspace
    // qemu_log("KMODE is: %d, curbuf at: %x", kmode, (int)((void*)curbuf));

    kmode = 1;
    curbuf = buffer;

    while(kmode == 1) {
        if (lastKey == 0x9C) { // Enter key pressed
            curbuf = 0;
            lastKey = 0;
            kmode = 0;
            chartyped = 0;
        }
    }
}

// Limited version of gets.
// Returns 0 if okay, returns 1 if you typed more than `length` keys.
int gets_max(char *buffer, int length) { // TODO: Backspace
    kmode = 1;
    curbuf = buffer;

    while(kmode == 1) {
        if(chartyped >= length) {
            curbuf = 0;
            lastKey = 0;
            kmode = 0;
            chartyped = 0;
            return 1;
        }

        if (lastKey == 0x9C) { // Enter key pressed
            curbuf = 0;
            lastKey = 0;
            kmode = 0;
            chartyped = 0;
        }
    }

    return 0;
}

/**
 * @brief Обработчик клавиатуры
 */
void keyboardHandler(registers_t regs){
    uint32_t kbdstatus = inb(PS2_STATE_REG);

    if (kbdstatus & 0x01) {
        lastKey = ps2_read();

        keyboard_buffer_put(lastKey);
    }
}

uint32_t getchar() {
    uint32_t character = 0;

    while(1) {
        uint32_t key = getkey();

        bool pressed = (key & 0x80);
        uint32_t keycode = key & 0x7F;

        if(pressed) {
            if(keycode == KEY_LSHIFT) {
                keyboard_states |= (KEYBOARD_STATE_SHIFT);
                continue;
            } else if(keycode == KEY_LCTRL) {
                keyboard_states |= (KEYBOARD_STATE_CTRL);
                continue;
            }
        } else {
            if(keycode == KEY_LSHIFT) {
                keyboard_states &= ~(KEYBOARD_STATE_SHIFT);
                continue;
            } else if(keycode == KEY_LCTRL) {
                keyboard_states |= (KEYBOARD_STATE_CTRL);
                continue;
            } else {
                if(key >= 256) {
                    qemu_err("Unknown key code: %d", key);
                    return 0;
                }

                // TODO: Multiple layouts
                character = (~keyboard_states & KEYBOARD_STATE_SHIFT) ?
                                    keyboard_layout[keycode] :
                                    shifted_keyboard_layout[keycode];
                break;
            }
        }
    }
    
    return character;
}
/**
 * @brief Выполняет инициализацию клавиатуры
 */
void ps2_keyboard_init() {
    uint8_t stat;

    ps2_in_wait_until_empty();

    outb(PS2_DATA_PORT, 0xf4);
    
    // ps2_write(0xf4);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Enable fail");
        return;
    }

    ps2_in_wait_until_empty();

    outb(PS2_DATA_PORT, 0xf0);

    // ps2_write(0xf0);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Scancode set fail");
        return;
    }

    ps2_in_wait_until_empty();

    outb(PS2_DATA_PORT, 0);
    // ps2_write(0);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Zero fail");
        return;
    }

    size_t scancode = ps2_read() & 0b11;

    qemu_note("SCANCODE SET: %d", scancode);

    ps2_in_wait_until_empty();

    outb(PS2_DATA_PORT, 0xf3);
    // ps2_write(0xf3);
    stat = ps2_read();

    if(stat != 0xfa) {
        qemu_err("Keyboard error: Repeat fail");
        return;
    }

    ps2_in_wait_until_empty();

    outb(PS2_DATA_PORT, 0);
    // ps2_write(0);
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
