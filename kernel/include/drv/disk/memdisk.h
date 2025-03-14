#pragma once

#include <common.h>

typedef struct {
    void* memory;
    size_t size;
} memdisk_t;

bool memdisk_create(char letter, void* memory, size_t size);
