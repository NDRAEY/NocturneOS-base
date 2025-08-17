#pragma once

#include <common.h>

typedef struct {
    void* memory;
    size_t size;
} memdisk_t;

bool memdisk_create(const char* preferred_id, void* memory, size_t size);
