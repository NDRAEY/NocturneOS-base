#pragma once

#define KEYBOARD_STATE_SHIFT (1 << 0)
#define KEYBOARD_STATE_CTRL (1 << 1)
#define KEYBOARD_STATE_ALT (1 << 2)

char* __getCharKeyboard(char* en_s,char* en_b,char* ru_s,char* ru_b);
char* getCharKeyboard(int key, bool mode);
void keyboardHandler(registers_t regs);
void keyboardctl(uint8_t param, bool value);
int getCharRaw();
void* getCharKeyboardWait(bool use_int);
void ps2_keyboard_init();
void gets(char *buffer);
bool is_lctrl_key();
int getCharRaw();
int getIntKeyboardWait();
uint8_t getPressReleaseKeyboard();
int gets_max(char *buffer, int length);

// Defined in keyboard_buffer.rs
extern void keyboard_buffer_init();
extern void keyboard_buffer_put(uint32_t keycode);
extern uint32_t keyboard_buffer_get();

SAYORI_INLINE uint32_t getkey() {
    return keyboard_buffer_get();
}

uint32_t getchar();