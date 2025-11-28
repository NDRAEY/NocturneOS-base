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

#ifdef NOCTURNE_SUPPORT_TIER1
extern volatile bool scheduler_working;
#endif

volatile size_t tick = 0;                /* Количество тиков */
volatile size_t frequency = CLOCK_FREQ;  /* Частота */

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
size_t getFrequency() {
    return frequency;
}

/**
 * @brief Ожидание по тикам
 *
 * @param delay - Тики
 */
void sleep_ticks(size_t delay){
    size_t current_ticks = getTicks();
    while (1) {
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
void sleep_ms(size_t milliseconds) {
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
void init_timer(size_t f) {
    __asm__ volatile("cli" ::: "memory");

    frequency = f;

    size_t divisor = BASE_FREQ / f;

    qemu_log("Divisor: %x", (uint32_t)divisor);

    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    // Channel 0 (bits 7:6), Access mode: lobyte/hibyte (bits 5:4), Operating mode: Rate generator (bits 3:1)
    outb(0x43, (0b00 << 6) | (0b11 << 4) | (0b010 << 1));

    outb(0x40, low);
    outb(0x40, high);

	register_interrupt_handler(IRQ0, timer_callback);

    __asm__ volatile("sti" ::: "memory");
}
