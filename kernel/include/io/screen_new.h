#pragma once

void screenman_init();
uint32_t screenman_add_screen(
    uint8_t* framebuffer,
    uint32_t width,
    uint32_t height,
    uint32_t pixel_format
);
uint32_t screenman_get_default_screen_index();
void screenman_get_screen_parameters(
    uint32_t index,
    uint32_t *out_width,
    uint32_t *out_height,
    uint32_t *out_size,
    uint32_t *out_bpp,
    uint8_t** out_framebuffer
);