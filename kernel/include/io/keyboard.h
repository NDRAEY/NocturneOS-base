#pragma once

#include <common.h>

#define KEYBOARD_STATE_PRESSED (1 << 0)
#define KEYBOARD_STATE_SHIFT (1 << 1)
#define KEYBOARD_STATE_CTRL (1 << 2)
#define KEYBOARD_STATE_ALT (1 << 3)

// Defined in keyboard_buffer.rs
extern void keyboard_buffer_init();
extern void keyboard_buffer_put(uint32_t keycode);
extern uint32_t keyboard_buffer_get();
extern uint32_t keyboard_buffer_get_or_nothing();

SAYORI_INLINE uint32_t getkey() {
    return keyboard_buffer_get();
}

uint32_t getchar();
void gets(char *buffer);