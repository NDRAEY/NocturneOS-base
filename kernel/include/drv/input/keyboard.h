#pragma once


void keyboardHandler(registers_t regs);
void* getCharKeyboardWait(bool use_int);
bool is_lctrl_key();
int getIntKeyboardWait();
uint8_t getPressReleaseKeyboard();
int gets_max(char *buffer, int length);

void ps2_keyboard_init();