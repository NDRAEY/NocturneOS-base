#include <common.h>
#include "gui/basics.h"
#include "io/screen.h"

void draw_rectangle(const uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    size_t ay = y+h, ax = x+w;

    for(size_t i = x, to = x+w; i < to; i++) {
        set_pixel(i, y, color);
        set_pixel(i, ay, color);
    }
	
    for(size_t j = y, to2 = y+h; j < to2; j++) {
        set_pixel(x, j, color);
        set_pixel(ax, j, color);
    }
}

void draw_filled_rectangle(size_t x, size_t y, size_t w, size_t h, uint32_t fill) {
    for(size_t i = 0; i < h; i++) {
        register size_t j = 0;

        for(; j < w; j += 4) {
            set_pixel4x1(x + j, y + i, fill);
        }

        j -= 4;

        for(; j < w; j++) {
            set_pixel(x+j, y+i, fill);
        }
    }
}
