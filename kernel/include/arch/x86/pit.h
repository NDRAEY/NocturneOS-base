#pragma once

#define BASE_FREQ 1193180
#define CLOCK_FREQ  1000

#include <common.h>

#define sleep(_d) sleep_ms((_d) * CLOCK_FREQ)
#define timestamp() ((getTicks() * 1000) / getFrequency())

size_t getTicks();
size_t getFrequency();
void sleep_ticks(size_t delay);
void sleep_ms(size_t milliseconds);
void init_timer(size_t f);
