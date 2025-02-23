#include <common.h>
#include <fs/fsm.h>

size_t fsm_DateConvertToUnix(FSM_TIME time) {
    uint32_t seconds_per_day = 24 * 60 * 60;
    size_t unix_time = 0;

    // Подсчет количества дней с начала Unix эпохи
    for (uint32_t year = 1970; year < time.year; year++) {
        uint32_t days_in_year = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
        unix_time += days_in_year * seconds_per_day;
    }

    int8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (time.year % 4 == 0 && (time.year % 100 != 0 || time.year % 400 == 0)) {
        month_days[1] = 29;
    }

    // Добавление количества дней в текущем году
    for (uint32_t month = 0; month < time.month - 1; month++) {
        unix_time += month_days[month] * seconds_per_day;
    }

    // Добавление количества дней в текущем месяце
    unix_time += (time.day - 1) * seconds_per_day;

    // Добавление компонентов времени
    unix_time += time.hour * 3600 + time.minute * 60 + time.second;

    return unix_time;
}


void fsm_convertUnix(uint32_t unix_time, FSM_TIME* time) {
    uint32_t seconds_per_day = 24 * 60 * 60;
    uint32_t days = unix_time / seconds_per_day;
    
    uint32_t years = 1970;
    uint32_t month, day;
    while (1) {
        uint32_t days_in_year = (years % 4 == 0 && (years % 100 != 0 || years % 400 == 0)) ? 366 : 365;
        if (days < days_in_year) {
            break;
        }
        days -= days_in_year;
        years++;
    }
    
    int8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (years % 4 == 0 && (years % 100 != 0 || years % 400 == 0)) {
        month_days[1] = 29;
    }
    
    uint32_t year_day = days;
    for (month = 0; month < 12; month++) {
        if (year_day < month_days[month]) {
            break;
        }
        year_day -= month_days[month];
    }
    day = year_day + 1;
    
    // Вычисляем компоненты времени
    uint32_t seconds = unix_time % seconds_per_day;
    uint32_t hour = seconds / 3600;
    uint32_t minute = (seconds % 3600) / 60;
    uint32_t second = seconds % 60;
	//qemu_log("%d.%d.%d %d:%d:%d",years, month, day, hour, minute, second);

	time->year = years;
	time->month = month;
	time->day = day;
	time->hour = hour;
	time->minute = minute;
	time->second = second;
}

char* fsm_timePrintable(FSM_TIME time){
	char* btime = 0;

	asprintf(&btime, "%04d.%02d.%02d %02d:%02d:%02d",
		time.year,
		time.month,
		time.day,
		time.hour,
		time.minute,
		time.second);

	return btime;
}
