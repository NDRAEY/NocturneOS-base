#pragma once

#include <sys/timer.h>

#ifdef NOCTURNE_X86
#include <sys/cpu_isr.h>
#endif

void qemu_printf(const char *text, ...);
void new_qemu_printf(const char *format, ...);

extern void (*default_qemu_printf)(const char *text, ...) __attribute__((format(printf, 1, 2)));

#ifdef RELEASE
#define qemu_note(M, ...) 
#define qemu_log(M, ...)
#define qemu_warn(M, ...)
#define qemu_ok(M, ...)
#define qemu_err(M, ...)
#else

#if __INTELLISENSE__
#define __FILE_NAME__  __FILE__
#endif

#define qemu_log(M, ...) default_qemu_printf("[LOG   %d] (%s:%s:%d) " M "\n", timestamp(), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_note(M, ...) default_qemu_printf("[\033[36;1mNOTE \033[33;0m %d] (%s:%s:%d) \033[36;1m" M "\033[0m\n", timestamp(), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_warn(M, ...) default_qemu_printf("[\033[33;1mWARN \033[33;0m %d] (%s:%s:%d) \033[33;1m" M "\033[0m\n", timestamp(), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_ok(M, ...) default_qemu_printf("[\033[32;1mOK   \033[33;0m %d] (%s:%s:%d) \033[32;1m" M "\033[0m\n", timestamp(), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define qemu_err(M, ...) default_qemu_printf("[\033[31;1mERROR\033[33;0m %d] (%s:%s:%d) \033[31;1m" M "\033[0m\n", timestamp(), __FILE_NAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#define assert(condition, format, ...) do { if (condition) {                 \
    qemu_printf("======================================\n");          \
    qemu_printf("[%s:%s:%d]  ASSERT FAILED: " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);   \
    qemu_printf("======================================\n");          \
    bsod_screen((registers_t){}, "ASSERT_FAIL", "See additional information on COM1 port. (Or Qemu.log if you're using QEMU)", 0xFFFF); \
} } while(0)

void switch_qemu_logging();
