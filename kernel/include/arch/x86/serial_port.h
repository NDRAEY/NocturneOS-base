#pragma once

#include "common.h"
#include <stdarg.h>

#define PORT_COM1 0x3f8
#define PORT_COM2 0x2F8
#define PORT_COM3 0x3E8
#define PORT_COM4 0x2E8
#define PORT_COM5 0x5F8
#define PORT_COM6 0x4F8
#define PORT_COM7 0x5E8
#define PORT_COM8 0x4E8

void __com_pre_formatString(int16_t port, const char* format, va_list args);
void __com_writeString(uint16_t port, char *buf);
void __com_formatString(int16_t port, char *text, ...);
void __com_setInit(uint16_t key, uint16_t value);
int __com_init(uint16_t port);