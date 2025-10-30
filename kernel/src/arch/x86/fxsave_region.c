char fxsave_region[512] __attribute__((aligned(16))) = {0};

// This should be used in context load/restore
void fpu_save() {
    __asm__ volatile("fxsave %0 " :: "m"(fxsave_region));
}

void fpu_restore() {
    __asm__ volatile("fxrstor %0 " :: "m"(fxsave_region));
}
