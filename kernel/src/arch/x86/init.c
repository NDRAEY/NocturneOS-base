#include "sys/sse.h"

extern void fpu_save();

void arch_init() {
    if (sse_check()) {
        fpu_save();
    }
}
