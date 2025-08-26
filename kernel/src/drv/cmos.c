/**
 * @file drv/cmos.c
 * @brief Драйвер CMOS
 * @author NDRAEY >_ (pikachu_andrey@vk.com)
 * @version 0.4.2
 * @date 2022-11-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include "io/ports.h"
#include "drv/cmos.h"

// FIXME: We need this?
#define CURRENT_CENTURY 20
 
uint16_t century_register = 0x00;     // Set by ACPI table parsing code if possible
 
unsigned char cmos_second;
unsigned char cmos_minute;
unsigned char cmos_hour;
unsigned char cmos_day;
unsigned char cmos_month;
unsigned short cmos_year;
unsigned char cmos_century;

unsigned char last_second;
unsigned char last_minute;
unsigned char last_hour;
unsigned char last_day;
unsigned char last_month;
unsigned char last_year;
unsigned char last_century;
unsigned char registerB;

enum {
    cmos_address = 0x70,
    cmos_data    = 0x71
};

/**
 * @brief Проверяет CMOS на обновление
 *
 * @return Если значение не равно нулю, значит CMOS обновляется
 */
SAYORI_INLINE uint8_t get_update_in_progress_flag() {
    outb(cmos_address, 0x0A);
    return (inb(cmos_data) & 0x80);
}

/**
 * @brief Получает регистр CMOS
 */
SAYORI_INLINE uint8_t get_RTC_register(int32_t reg) {
    outb(cmos_address, reg);
    return inb(cmos_data);
}

static inline void rtc_wait_update() {
    while(get_update_in_progress_flag()) {
        __asm__ volatile("hlt");
    }
}

/**
 * @brief Считывает время с CMOS
 */
void read_rtc() {
    rtc_wait_update();

    cmos_second = get_RTC_register(0x00);
    cmos_minute = get_RTC_register(0x02);
    cmos_hour = get_RTC_register(0x04);
    cmos_day = get_RTC_register(0x07);
    cmos_month = get_RTC_register(0x08);
    cmos_year = get_RTC_register(0x09);

	if(century_register != 0) {
        cmos_century = get_RTC_register(century_register);
    }
 
    do {
        last_second = cmos_second;
        last_minute = cmos_minute;
        last_hour = cmos_hour;
        last_day = cmos_day;
        last_month = cmos_month;
        last_year = cmos_year;
        last_century = cmos_century;
 
        rtc_wait_update();

		cmos_second = get_RTC_register(0x00);
        cmos_minute = get_RTC_register(0x02);
        cmos_hour = get_RTC_register(0x04);
        cmos_day = get_RTC_register(0x07);
        cmos_month = get_RTC_register(0x08);
        cmos_year = get_RTC_register(0x09);
        if(century_register != 0) {
            cmos_century = get_RTC_register(century_register);
        }
    } while( (last_second != cmos_second) || (last_minute != cmos_minute) || (last_hour != cmos_hour) ||
           (last_day != cmos_day) || (last_month != cmos_month) || (last_year != cmos_year) ||
           (last_century != cmos_century) );
 
    registerB = get_RTC_register(0x0B);
 
    // Convert BCD to binary values if necessary
 
    if (!(registerB & 0x04)) {
        cmos_second = (cmos_second & 0x0F) + ((cmos_second >> 4) * 10);
        cmos_minute = (cmos_minute & 0x0F) + ((cmos_minute >> 4) * 10);
        cmos_hour = ( (cmos_hour & 0x0F) + (((cmos_hour & 0x70) >> 4) * 10) ) | (cmos_hour & 0x80);
        cmos_day = (cmos_day & 0x0F) + ((cmos_day >> 4) * 10);
        cmos_month = (cmos_month & 0x0F) + ((cmos_month >> 4) * 10);
        cmos_year = (cmos_year & 0x0F) + ((cmos_year >> 4) * 10);

        if(century_register != 0) {
            cmos_century = (cmos_century & 0x0F) + ((cmos_century >> 4) * 10);
        }
    }
 
    // Convert 12-hour clock to 24-hour clock if necessary
 
    if (!(registerB & 0x02) && (cmos_hour & 0x80)) {
        cmos_hour = ((cmos_hour & 0x7F) + 12) % 24;
    }
 
    // Calculate the full (4-digit) year
 
    if(century_register != 0) {
        cmos_year += cmos_century * 100;
    } else {
        cmos_year += CURRENT_CENTURY * 100;
    }
}

/**
 * @brief Считывает время и передает в удобной структуре
 */
sayori_time_t get_time() {
    read_rtc();

    struct sayori_time time = {
		cmos_second, cmos_minute, cmos_hour, cmos_day, cmos_month, cmos_year, cmos_century
	};
	
    return time;
}
