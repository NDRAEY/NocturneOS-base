/**
 * @file drv/psf.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), Арен Елчинян (SynapseOS)
 * @brief Поддержка шрифтов PSF
 * @version 0.3.5
 * @date 2023-01-13
 * @copyright Copyright SayoriOS Team (c) 2023
 */

#include <drv/psf.h>
#include <lib/stdio.h>
#include <io/ports.h>
#include "mem/vmm.h"
#include "io/serial_port.h"
#include "io/screen.h"

uint32_t psf_font_version = 0;

static psf_t *_font_ptr = nullptr;
static bool _init = false;
static uint8_t _h = 0;
static uint8_t* first_glyph = 0;

uint16_t *unicode;

/**
 * @brief Инициализация шрифта PSF
 * @param psf - (const char*) имя файла
 * @return true - всё ок; false - ошибка
 */
bool text_init(char* psf){
	ON_NULLPTR(psf, {
		qemu_log("Filename is nullptr!");
		return false;
	});

    FILE* psf_file = fopen(psf, "r");
    if (!psf_file) {
        qemu_log("[Core] [PSF] Не удалось найти файл `%s`. \n",psf);
        return false;
    }

    fseek(psf_file, 0, SEEK_END);
    size_t rfsize = ftell(psf_file);
    fseek(psf_file, 0, SEEK_SET);

    char* buffer = kmalloc(rfsize);
	fread(psf_file, rfsize, 1, buffer);
    fclose(psf_file);

    psf_t *header = (psf_t*)buffer;
    _init = false;
    _h = 0;
    if (header->magic[0] != PSF1_MAGIC0 || header->magic[1] != PSF1_MAGIC1){
        qemu_log("PSF Header Error");
        return false;
    }
    _font_ptr = (psf_t*)buffer;
    _h = header->charHeight;
    _init = true;
    first_glyph = (uint8_t*)_font_ptr+sizeof(psf_t);
    return _init;
}

size_t psf1_get_h(){
    return _h;
}
/// 252
uint16_t psf1_rupatch(uint16_t c,uint16_t c2){
    if (!isUTF(c)) return c;
    if ((c & 0x1F) != 16 && (c & 0x1F) != 17)
        return c;

    uint8_t lo = (uint8_t)(c2 & 0x3F);
    uint8_t hi = (uint8_t)(c);
    
    uint16_t t = 0;

    if(hi == 0xd0 || hi == 0xd1) {
        uint16_t sym = lo & 0x3f;

        if(sym == 0) {
            t = 224;
        } else if(sym == 1) {
            t = hi == 0xd0 ? 240 : 225;
        } else if(sym >= 2 && sym <= 15) {
            t = 224 + sym;
        } else if(sym == 16) {
            t = 128;
        } else if(sym >= 18 && sym <= 63) {
            t = 128 + (sym - 16);
        } else if(sym == 17) {
            t = hi == 0xd1 ? 241 : 129;
        }
    }

    return t;
}

uint8_t *psf1_get_glyph(uint16_t ch){
    psf_t *header = (psf_t*)_font_ptr;

    if ((ch > 511) || (ch > 255 && (header->mode == 0 || header->mode == 2))){
        return 0;
    }

    return ((uint8_t*)_font_ptr+sizeof(psf_t)+(ch*_h));
}

void draw_vga_ch(uint16_t c, uint16_t c2, size_t pos_x, size_t pos_y, size_t color) {
    // char mask[8] = {128,64,32,16,8,4,2,1};
    uint8_t *glyph = psf1_get_glyph(psf1_rupatch(c, c2));

	if(!glyph)
		return;

    for (size_t y = 0; y < _h; y++) {
        size_t rposy = pos_y + y;
        for (register size_t x = 0; x < 8; x++){
            if (glyph[y] & (1 << (8 - x))) {
            // if (glyph[y] & mask[x]) {
                set_pixel(pos_x+x, rposy, color);
            }
        }
    }
}

void draw_vga_str(const char* text, size_t len, int x, int y, uint32_t color){
	ON_NULLPTR(text, {
		return;
	});

    size_t scrwidth = getScreenWidth();
    for(int i = 0; i < len; i++){
        if (x + 8 <= scrwidth){
            if (isUTF(text[i])){
                draw_vga_ch(text[i], text[i+1], x, y, color);
                i++;
            } else {
                if(!text[i])
                    return;
                draw_vga_ch(text[i], 0, x, y, color);
            }
            x += 8;
        } else {
            break;
        }
    }
}
