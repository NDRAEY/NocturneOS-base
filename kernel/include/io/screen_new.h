#pragma once

void screen_init(
    uint8_t* framebuffer,
    uint32_t width,
    uint32_t height,
    uint32_t pixel_format
);
void screen_get(
    uint32_t out_width,
    uint32_t out_height,
    size_t out_size,
    uint32_t out_bpp,
    uint8_t** out_framebuffer
);
void screen_set(uint32_t width, uint32_t height, uint32_t bpp, uint8_t* framebuffer);