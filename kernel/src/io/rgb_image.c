//
// Created by ndraey on 25.10.23.
//

#include <io/screen.h>
#include "io/rgb_image.h"
#include "io/ports.h"

/**
 * @brief Рисует изображение в виде массива байт в видеопамяти.
 * @param data Массив данных изображения
 * @param width Ширина изображения
 * @param height Высота изображения
 * @param bpp Количество бит на пиксель
 * @param sx Начальная координата X
 * @param sy Начальная координата Y
 */
void draw_rgb_image(const char *data, size_t width, size_t height, size_t bpp, int sx, int sy) {
	size_t x = 0, y = 0;

    size_t bytes_pp = bpp >> 3;

	while(y < height) {
		size_t prepos = (width * bytes_pp) * y;
		x = 0;
		while(x < width) {
			int px = prepos + (x * bytes_pp);

			char r = data[px];
			char g = data[px + 1];
			char b = data[px + 2];

			size_t color = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);

			set_pixel(sx + x, sy + y, color);

			x++;
		}
		y++;
	}
}