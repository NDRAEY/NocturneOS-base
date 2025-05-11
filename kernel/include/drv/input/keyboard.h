#pragma once

#include "sys/registers.h"

void keyboardHandler(registers_t regs);
void* getCharKeyboardWait(bool use_int);
bool is_lctrl_key();
int getIntKeyboardWait();
uint8_t getPressReleaseKeyboard();
int gets_max(char *buffer, int length);

void ps2_keyboard_init();
void ps2_keyboard_install_irq();