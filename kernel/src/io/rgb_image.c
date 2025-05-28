//
// Created by ndraey on 25.10.23.
//

#include <io/screen.h>
#include "io/rgb_image.h"
#include "io/ports.h"
#include <emmintrin.h>

// TODO: Care about remaining pixels of image.
__attribute__((force_align_arg_pointer)) void draw_rgb_image(const char *data, size_t width, size_t height, size_t bpp, int sx, int sy) {
	size_t x = 0, y = 0;

	size_t bytes_pp = bpp >> 3;
	size_t line_size = width * bytes_pp;

	const char* dp = back_framebuffer_addr + sy * framebuffer_pitch;
	const size_t fb_bpp = framebuffer_bpp >> 3;

	while(y < height) {
		size_t prepos = line_size * y;
		x = 0;
		while(x < width) {
			size_t pos = prepos + (x * bytes_pp);
			__m128i pix4 = _mm_loadu_si128((__m128i*)(data + pos));
			_mm_storeu_si128((__m128i*)(dp + (sx * fb_bpp) + pos), pix4);

			x += 4;
		}
		y++;
	}
}
