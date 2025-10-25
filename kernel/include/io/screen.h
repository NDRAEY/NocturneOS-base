#pragma once

#include <common.h>
#include "multiboot.h"

extern uint8_t* framebuffer_addr;
extern volatile uint32_t framebuffer_bpp;
extern volatile uint32_t framebuffer_pitch;
extern uint8_t* back_framebuffer_addr;
extern volatile uint32_t framebuffer_size;

typedef enum {
	SCREEN_QUERY_WIDTH = 0,
	SCREEN_QUERY_HEIGHT = 1,
	SCREEN_QUERY_BITS_PER_PIXEL = 2,
} screen_query_t;

#define VESA_WIDTH  (getScreenWidth())
#define VESA_HEIGHT (getScreenHeight())

#define PACK_INTO_RGB(struct_px) ((struct_px.r & 0xff) << 16)  |\
                                 ((struct_px.g & 0xff) << 8) |\
                                  (struct_px.b & 0xff)

typedef struct rgba_struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} rgba_color;

typedef struct svga_mode_info {
	uint16_t attributes;
	uint8_t windowA, windowB;
	uint16_t granularity;
	uint16_t windowSize;
	uint16_t segmentA, segmentB;
	uint32_t winFuncPtr;
	uint16_t pitch;

	uint16_t screen_width, screen_height;
	uint8_t wChar, yChar, planes, bpp, banks;
	uint8_t memoryModel, bankSize, imagePages;
	uint8_t reserved0;

	// Color masks
	uint8_t readMask, redPosition;
	uint8_t greenMask, greenPosition;
	uint8_t blueMask, bluePosition;
	uint8_t reservedMask, reservedPosition;
	uint8_t directColorAttributes;

	uint32_t physbase;
	uint32_t offScreenMemOff;
	uint16_t offScreenMemSize;
	uint8_t reserved1[206];
} __attribute__ ((packed)) svga_mode_info_t;

uint32_t getDisplayPitch();
uint32_t getScreenWidth();
uint32_t getScreenHeight();
size_t getDisplayAddr();
uint32_t getDisplayBpp();
size_t getFrameBufferAddr();
size_t getPixel(int32_t x, int32_t y);


/**
 * @brief Вывод одного пикселя на экран
 *
 * @param x - позиция по x
 * @param y - позиция по y
 * @param color - цвет
 */
inline static __attribute__((always_inline)) void set_pixel(uint32_t x, uint32_t y, uint32_t color) {
	if (x >= VESA_WIDTH ||
		y >= VESA_HEIGHT) {
		return;
	}
	
	uint8_t* pixels = back_framebuffer_addr + (x * (framebuffer_bpp >> 3)) + y * framebuffer_pitch;

	pixels[0] = color & 0xff;
	pixels[1] = (color >> 8) & 0xff;
	pixels[2] = (color >> 16) & 0xff;
}

inline static __attribute__((always_inline)) void set_pixel4x1(uint32_t x, uint32_t y, uint32_t color) {
	uint8_t* pixels = back_framebuffer_addr + (x * (framebuffer_bpp >> 3)) + y * framebuffer_pitch;

	pixels[0] = pixels[4] = pixels[8] = pixels[12] = color & 0xff;
	pixels[1] = pixels[5] = pixels[9] = pixels[13] = (color >> 8) & 0xff;
	pixels[2] = pixels[6] = pixels[10] = pixels[14] = (color >> 16) & 0xff;
}

/**
 * @brief Получение размера буфера экрана
 *
 * @return uint32_t - Размер буфера экрана
 */
inline static __attribute__((always_inline)) uint32_t getDisplaySize(){
	return framebuffer_size;
}

void setPixelAlpha(uint32_t x, uint32_t y, rgba_color color);
void rect_copy(int x, int y, int width, int height);
void graphics_update(uint32_t new_width, uint32_t new_height, uint32_t new_pitch);

void init_vbe(const multiboot_header_t *mboot);
void screen_update();

void clean_screen();
