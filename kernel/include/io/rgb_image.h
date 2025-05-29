//
// Created by ndraey on 25.10.23.
//

#pragma once

#include "common.h"

#define PIXIDX(w, x, y) ((w) * (y) + (x))

void draw_rgb_image(const uint8_t *data, size_t width, size_t height, size_t bpp, int sx, int sy);