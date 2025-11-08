/**
 * @file sys/timer.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Модуль системного таймера
 * @version 0.4.2
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */
#include  "arch/x86/pit.h"
#include  "arch/x86/isr.h"
#include  "arch/x86/ports.h"
#include "io/logging.h"
#include "sys/scheduler.h"

extern bool scheduler_working;

volatile size_t tick = 0;                /* Количество тиков */
size_t frequency = CLOCK_FREQ;  /* Частота */

/**
 * @brief Получить количество тиков
 *
 * @return size_t - Количество тиков с момента старта
 */
size_t getTicks(){
    return (size_t)tick;
}

double getUptime() {
    if(getFrequency() == 0) {
        return 0.0;
    }else{
        return (double)tick / (double)frequency;
    }
}

/**
 * @brief Получить частоту таймера
 *
 * @return uint32_t - Частота таймера
 */
size_t getFrequency(){
    return frequency;
}

/**
 * @brief Ожидание по тикам
 *
 * @param delay - Тики
 */
void sleep_ticks(uint32_t delay){
    size_t current_ticks = getTicks();
    while (1){
        if (current_ticks + delay < getTicks()){
            break;
        } else {
        	__asm__ volatile("nop");
            yield();
        }
    }
}

/**
 * @brief Ожидание по миллисекундам
 *
 * @param milliseconds - Миллисекунды
 */
void sleep_ms(uint32_t milliseconds) {
    uint32_t needticks = milliseconds * frequency;
    sleep_ticks(needticks / 1000);
}

/**
 * @brief Таймер Callback
 *
 * @param regs - Регистры процессора
 */
void timer_callback(SAYORI_UNUSED registers_t regs){
    tick++;

    #ifdef NOCTURNE_SUPPORT_TIER1
    if (is_multitask() && scheduler_working) {
        task_switch_v2_wrapper(regs);
    }
    #endif
}

/**
 * @brief Инициализация модуля системного таймера
 *
 * @param - Частота
 */
void init_timer(uint32_t f){
    frequency = f;

    uint32_t divisor = BASE_FREQ / f;

    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    outb(0x40, low);
    outb(0x40, high);

	register_interrupt_handler(IRQ0, &timer_callback);

    __asm__ volatile("sti");
}
