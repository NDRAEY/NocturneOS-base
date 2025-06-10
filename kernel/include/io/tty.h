#pragma once

#include <common.h>
#include "io/screen.h"
#include <stdarg.h>
#include "generated/tty_headers.h"

void tty_puts(const char* str);
void tty_puts_raw(const char* str, size_t length);
void tty_putchar(char c);
void _tty_print(const char *format, va_list args);
void _tty_printf(const char *text, ...);

#define tty_print(format, args) _tty_print(format, args); screen_update()

static inline void tty_printf(char *text, ...) {
    va_list args;
    va_start(args, text);
    tty_print(text, args);
    va_end(args);
}

void drawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

void tty_setcolor(uint32_t color);
uint32_t tty_getcolor();

void tty_set_bgcolor(uint32_t color);

void set_cursor_enabled(bool en);

void tty_clear();
void buffer_set_pixel4(uint8_t *buffer, size_t width, size_t height, size_t x, size_t y, size_t color);

void tty_backspace();

void tty_taskInit();

uint32_t tty_get_pos_x();
uint32_t tty_get_pos_y();