#pragma once

#include <common.h>
#include "sys/registers.h"
#include "sys/cpu_isr.h"

void insw(uint16_t __port, void *__buf, unsigned long __n);
void outsw(uint16_t __port, const void *__buf, unsigned long __n);
// void insl(uint16_t reg, uint32_t *buffer, int32_t quads);
void outsl(uint16_t reg, const uint32_t *buffer, int32_t quads);

int32_t com1_is_transmit_empty();
int32_t is_transmit_empty(uint16_t port);

uint8_t serial_readchar(uint16_t port);
void io_wait();

void switch_qemu_logging();
