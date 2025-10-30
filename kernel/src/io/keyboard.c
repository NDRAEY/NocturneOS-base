#include <common.h>
#include <io/logging.h>
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

    // TODO: Can be simplified
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
            keyboard_states |= (KEYBOARD_STATE_CTRL);
        } else if(keycode == KEY_LEFT) {
            character = 0x00445b1b;
        } else if(keycode == KEY_RIGHT) {
            character = 0x00435b1b;
        } else if(keycode == KEY_UP) {
            character = 0x00415b1b;
        } else if(keycode == KEY_DOWN) {
            character = 0x00425b1b;
        } else if(keycode == KEY_DELETE) {
            character = 0x7e335b1b;
        }   else {
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
        } else if(keycode == KEY_LCTRL) {
            keyboard_states &= ~(KEYBOARD_STATE_CTRL);
        } 
    }

    if((keyboard_states & KEYBOARD_STATE_CTRL) && character == 's') {
        return 0x13;
    }

    if((keyboard_states & KEYBOARD_STATE_CTRL) && character == 'q') {
        return 0x11;
    }

    if((keyboard_states & KEYBOARD_STATE_CTRL) && character == 'o') {
        return 0x0f;
    }

    return character;
}

uint32_t getchar() {
    while(true) {
        uint32_t key = getkey();
        uint32_t ch = parse_char(key);

        if(ch != 0) {
            return ch;
        }
    }
}
