#pragma once

#include	"common.h"

typedef struct {
    volatile bool lock;
} mutex_t;

/* Get mutex */
void mutex_get(mutex_t* mutex);

/* Release mutex */
void mutex_release(mutex_t* mutex);
