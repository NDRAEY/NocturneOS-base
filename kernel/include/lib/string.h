#pragma once

#include "common.h"

int32_t memcmp(const char *s1, const char *s2, size_t n);
void* memcpy(void *restrict destination, const void *restrict source, size_t n);
void* memset(void* ptr, int value, size_t num);
void* memmove(void *dest, void *src, size_t count);

char *strcat(char *s, const char *t);
size_t strspn(const char *s, const char *accept);
char* strcpy(char* dest, const char* src);
size_t strlen(const char *str);
size_t mb_strlen(const char *str);
void substr(char* restrict dest, const char* restrict source, int from, int length);
int strcmp(const char *s1, const char *s2);
int32_t strncmp(const char *s1, const char *s2, size_t num);

char *strchr(const char *_s, char _c);

bool isUTF(char c);

void sse_memcpy(void* restrict dest, const void* restrict src, size_t size);

char *strncpy(char *dest, const char *src, size_t n);

int atoi(const char* s);

SAYORI_INLINE bool isdigit(char a) {
	return a >= '0' && a <= '9';
}

SAYORI_INLINE
bool isalnum(char c) {
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z');
}

char* strdynamize(const char* str);
