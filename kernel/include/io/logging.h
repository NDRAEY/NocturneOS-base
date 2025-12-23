#pragma once

#include <sys/timer.h>

#ifdef NOCTURNE_X86
#include <sys/cpu_isr.h>
#endif

void qemu_printf(const char *text, ...);

extern void (*default_qemu_printf)(const char *text, ...) __attribute__((format(printf, 1, 2)));

#ifdef RELEASE
// I use it to debug in optimized builds.
// #if 0
#define qemu_note(M, ...) 
#define qemu_log(M, ...)
#define qemu_warn(M, ...)
#define qemu_ok(M, ...)
#define qemu_err(M, ...)
#else

#if __INTELLISENSE__
#define __FILE_NAME__  __FILE__
#endif

#define qemu_log(M, ...) default_qemu_printf("[LOG   %u] (%s:%s:%d) " M "\n", (uint32_t)(timestamp()), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_note(M, ...) default_qemu_printf("[\033[36;1mNOTE \033[33;0m %u] (%s:%s:%d) \033[36;1m" M "\033[0m\n", (uint32_t)(timestamp()), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_warn(M, ...) default_qemu_printf("[\033[33;1mWARN \033[33;0m %u] (%s:%s:%d) \033[33;1m" M "\033[0m\n", (uint32_t)(timestamp()), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_ok(M, ...) default_qemu_printf("[\033[32;1mOK   \033[33;0m %u] (%s:%s:%d) \033[32;1m" M "\033[0m\n", (uint32_t)(timestamp()), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_err(M, ...) default_qemu_printf("[\033[31;1mERROR\033[33;0m %u] (%s:%s:%d) \033[31;1m" M "\033[0m\n", (uint32_t)(timestamp()), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

// FIXME: Add `get_regs()` function.
#define assert(condition, format, ...) do { if (condition) {                 \
    qemu_printf("======================================\n");          \
    qemu_printf("[%s:%s:%d]  ASSERT FAILED: " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    qemu_printf("======================================\n");          \
    \
    registers_t regs; \
    get_regs(&regs);\
    bsod_screen(&regs, "ASSERT_FAIL", "See additional information on COM1 port. (Or Qemu.log if you're using QEMU)", 0xFFFF); \
} } while(0)
