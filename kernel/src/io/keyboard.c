#include <common.h>
#include <io/ports.h>
#include <io/tty.h>
#include <io/keyboard.h>
#include <lib/keymap.h>

volatile uint32_t keyboard_states = 0;
static const uint8_t keyboard_layout[128] = {
    0,    0,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',   0x7f,   0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',    '\n',   0,  'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0, ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0, 0, 0, 0, 0,
};

// Определяем таблицу символов для клавиш при зажатой клавише Shift
static const uint8_t shifted_keyboard_layout[128] = {
     0,   0,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',   0x7f,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  0,   0,  'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0, ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0, 0, 0, 0, 0,
};

uint32_t parse_char(uint32_t key) {
    uint32_t character = 0;

    bool pressed = (~key & 0x80);

    // Can be simplified
    if(pressed) {
        keyboard_states |= (KEYBOARD_STATE_PRESSED);
    } else {
        keyboard_states &= ~(KEYBOARD_STATE_PRESSED);
    }

    uint32_t keycode = key & 0x7F;

    if(pressed) {
        if(keycode == KEY_LSHIFT) {
            keyboard_states |= (KEYBOARD_STATE_SHIFT);
        } else if(keycode == KEY_LCTRL) {
            keyboard_states ^= (KEYBOARD_STATE_CTRL);
        } else {
            if(key >= 256) {
                qemu_err("Unknown key code: %d", key);
                return 0;
            }

            // TODO: Multiple layouts
            character = (~keyboard_states & KEYBOARD_STATE_SHIFT) ?
                                keyboard_layout[keycode] :
                                shifted_keyboard_layout[keycode];
        }
    } else {
        if(keycode == KEY_LSHIFT) {
            keyboard_states &= ~(KEYBOARD_STATE_SHIFT);
        }
    }

    return character;
}

uint32_t getchar() {
    // uint32_t character = 0;

    // while(1) {
    //     uint32_t key = getkey();

    //     bool pressed = (~key & 0x80);

    //     // Can be simplified
    //     if(pressed) {
    //         keyboard_states |= (KEYBOARD_STATE_PRESSED);
    //     } else {
    //         keyboard_states &= ~(KEYBOARD_STATE_PRESSED);
    //     }

    //     uint32_t keycode = key & 0x7F;

    //     if(pressed) {
    //         if(keycode == KEY_LSHIFT) {
    //             keyboard_states |= (KEYBOARD_STATE_SHIFT);
    //         } else if(keycode == KEY_LCTRL) {
    //             keyboard_states ^= (KEYBOARD_STATE_CTRL);
    //         } else {
    //             if(key >= 256) {
    //                 qemu_err("Unknown key code: %d", key);
    //                 return 0;
    //             }

    //             // TODO: Multiple layouts
    //             character = (~keyboard_states & KEYBOARD_STATE_SHIFT) ?
    //                                 keyboard_layout[keycode] :
    //                                 shifted_keyboard_layout[keycode];

    //             break;
    //         }
    //     } else {
    //         if(keycode == KEY_LSHIFT) {
    //             keyboard_states &= ~(KEYBOARD_STATE_SHIFT);
    //         }
    //     }
    // }

    // return character;

    while(true) {
        uint32_t key = getkey();
        qemu_printf("Key: %d (%x)\n", key, key);
        uint32_t ch = parse_char(key);

        if(ch != 0) {
            return ch;
        }
    }
}

void gets(char *buffer) {
    char* buf = buffer;

    while(1) {
        uint32_t ch = getchar();
        char* codes = (char*)&ch;

        if(ch == '\n') {
            break;
        }

        if(ch == '\b') {
            *buf-- = 0;
            tty_backspace();
            continue;
        }

        if(ch > 255) {
            *buf++ = codes[0];
            *buf++ = codes[1];
        } else {
            *buf++ = codes[0];
        }

        tty_printf("%c", ch);
    }
}
