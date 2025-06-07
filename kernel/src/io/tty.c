/**
 * @file io/tty.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Средства для работы с видеодрайвером
 * @version 0.3.5
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include <stdarg.h>
#include <mem/vmm.h>
#include <io/tty.h>
#include <sys/scheduler.h>
#include <io/ports.h>
#include <io/status_loggers.h>
#include <drv/fpu.h>
#include <lib/math.h>
#include <io/rgb_image.h>
#include "lib/sprintf.h"
#include "lib/asprintf.h"
#include "drv/psf.h"

// TODO: Eurica! Split tty.c into 2 files:
//       tty.c - only text processing functions
//       default_console.c - TTY client

// TODO: Keep here.
volatile uint8_t tty_feedback = 1;		/// ...
uint32_t tty_pos_x = 0;						/// Позиция на экране по X
uint32_t tty_pos_y = 0;						/// Позиция на экране по Y
int32_t tty_off_pos_x = 8;					/// ...
int32_t tty_off_pos_p = 0;					/// ...
uint32_t tty_off_pos_h = 16;					/// ...
uint32_t tty_text_color;				/// Текущий цвет шрифта
uint32_t tty_bg_color;                  /// Текущий задний фон
bool stateTTY = true;					/// Статус, разрешен ли вывод текста через tty_printf
static bool tty_autoupdate = true;
/////////////////////////////////

// TODO: Move to things/cursor.c
thread_t* threadTTY01 = nullptr;					/// Поток с анимацией курсора
volatile bool showAnimTextCursor = true;		/// Отображать ли анимацию курсора
void animTextCursor();
////////////////////////////////

/**
 * @brief Инициализация потоков
 */
void tty_taskInit() {
    qemu_log("Starting task...");
    process_t* proc = get_current_proc();
    qemu_log("Process at: %p", proc);
    threadTTY01 = thread_create(proc,
			   &animTextCursor,
			   0x4000,
			   true);
}

void tty_changeState(bool state){
    stateTTY = state;
}

void set_cursor_enabled(bool en) {
    showAnimTextCursor = en;
}

/**
 * @brief Изменение цвета текста
 *
 * @param color - цвет
 */
void tty_setcolor(uint32_t color) {
    tty_text_color = color;
}

uint32_t tty_getcolor() {
    return tty_text_color;
}

/**
 * @brief Изменение цвета заднего фона
 *
 * @param color - цвет
 */
void tty_set_bgcolor(uint32_t color) {
    tty_bg_color = color;
}

/**
 * @brief Изменяем позицию курсора по X
 *
 * @param x - позиция по X
 */
void setPosX(uint32_t x){
    tty_pos_x = x;
}


/**
 * @brief Изменяем позицию курсора по Y
 *
 * @param y - позиция по Y
 */
void setPosY(uint32_t y){
    tty_pos_y = y;
}

/**
 * @brief Устновливает пиксель RGB в буфере в котором все пиксели представляют собой RGBA (альфа канал игнорируется)
 * @param buffer - буфер RGBA
 * @param width - длина кадра который представляет буфер
 * @param height - ширина кадра который представляет буфер
 * @param x - координата x
 * @param y - координата у
 * @param color - цвет в формате RGB
 */
void buffer_set_pixel4(uint8_t *buffer, size_t width, size_t height, size_t x, size_t y, size_t color) {
    if(x >= width || y >= height)
        return;
    
    size_t pixpos = PIXIDX(width * 4, x * 4, y);

    buffer[pixpos + 0] = (uint8_t)color;
    buffer[pixpos + 1] = (uint8_t)(color >> 8);
    buffer[pixpos + 2] = (uint8_t)(color >> 16);
}

/**
 * @brief Удаление последнего символа
 *
 */
void tty_backspace() {
    if (tty_pos_x < (uint32_t)tty_off_pos_x) {
        if (tty_pos_y >= tty_off_pos_h) {
            tty_pos_y -= tty_off_pos_h;
        }
        tty_pos_x -= 1;
    } else {
        tty_pos_x -= tty_off_pos_x;
    }

	drawRect(tty_pos_x, tty_pos_y, tty_off_pos_x, tty_off_pos_h, 0x000000);
    punch();
}

/**
 * @brief Подфункция-обработчик для tty_printf
 *
 * @param format - строка форматов
 * @param args - аргументы
 */
void _tty_print(const char* format, va_list args) {    
    char* a = 0;

	vasprintf(&a, format, args);

	tty_puts(a);

	kfree(a);
}

void _tty_printf(const char *text, ...) {
    int sAT = (showAnimTextCursor?1:0);
    if (sAT == 1){
		showAnimTextCursor = false;
	}
    if (stateTTY){
        va_list args;
        va_start(args, text);
        _tty_print(text, args);
        va_end(args);
    }

	if (sAT == 1){
		showAnimTextCursor = true;
	}
}

/**
 * @brief Анимация курсора (для tty)
 */
void animTextCursor(){
    qemu_log("animTextCursor Work...");
    volatile bool vis = false;
    int ox = 0, oy = 0;

    while(1) {
        if(!showAnimTextCursor)
			continue;

		ox = tty_get_pos_x();
        oy = tty_get_pos_y();

        if (!vis){
            drawRect(ox,oy+tty_off_pos_h-3,tty_off_pos_x,3,0x333333);
            vis = true;
        } else {
            drawRect(ox,oy+tty_off_pos_h-3,tty_off_pos_x,3,0x000000);
            vis = false;
        }

		punch();
        sleep_ms(500);
    }

	// So, it never quits
//    qemu_log("animTextCursor complete...");
//    thread_exit(threadTTY01);
}

void screen_update() {
    punch();
}


void tty_set_autoupdate(bool value) {
    tty_autoupdate = value;
}

bool tty_get_autoupdate() {
    return tty_autoupdate;
}