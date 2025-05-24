#pragma once

#include "common.h"

typedef struct stackframe {
  struct stackframe* ebp;
  uint32_t eip;
} stackframe;

#ifndef RELEASE
void unwind_stack(uint32_t max_frames);
#else
void unwind_stack(uint32_t max_frames);
#endif
