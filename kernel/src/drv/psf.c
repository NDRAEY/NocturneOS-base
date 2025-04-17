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
#include "debug/hexview.h"

extern volatile void* PSF_FONT;

extern void psf_init(uint8_t* data, uint32_t len);

bool fonts_init(char* psf) {
	ON_NULLPTR(psf, {
		qemu_log("Filename is nullptr!");
		return false;
	});

    FILE* psf_file = fopen(psf, "r");

    if (!psf_file) {
        qemu_log("[Core] [PSF] Не удалось найти файл `%s`. \n",psf);
        return false;
    }

    size_t rfsize = fsize(psf_file);

    char* buffer = kmalloc(rfsize);
	fread(psf_file, rfsize, 1, buffer);
    fclose(psf_file);

    // hexview_advanced(buffer, rfsize, 24, false, new_qemu_printf);

    psf_init(buffer, rfsize);

    kfree(buffer);

    return true;
}

void draw_vga_str(const char* text, size_t len, int x, int y, uint32_t color) {
	ON_NULLPTR(text, {
		return;
	});

    size_t scrwidth = getScreenWidth();
    for(int i = 0; i < len; i++) {
        if (x + 8 <= scrwidth) {
            uint16_t ch = (uint16_t)(uint8_t)text[i];
            if(ch == 0xd0 || ch == 0xd1) {
                i++;

                uint16_t ch2 = (uint16_t)(uint8_t)text[i];
                ch <<= 8;

                ch |= ch2;
            }

            rs_draw_character(&PSF_FONT, ch, x, y, color);
            x += 8;
        } else {
            break;
        }
    }
}

void draw_character(uint16_t ch, size_t pos_x, size_t pos_y, uint32_t color) {
    rs_draw_character(&PSF_FONT, ch, pos_x, pos_y, color);
}
