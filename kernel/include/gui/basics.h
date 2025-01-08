#pragma once 

#include <common.h>
#include "io/screen.h"

void draw_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void draw_filled_rectangle(size_t x, size_t y, size_t w, size_t h, uint32_t fill);

SAYORI_INLINE void fill_screen(const uint32_t color) {
    draw_filled_rectangle(0, 0, VESA_WIDTH, VESA_HEIGHT, color);
}

