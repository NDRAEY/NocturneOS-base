/**
 * @file sys/syscalls.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Интерфейс системных вызовов
 * @version 0.3.5
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include	"sys/syscalls.h"
#include	"io/ports.h"
#include "io/screen.h"
#include	"io/tty.h"
#include	"user/env.h"
#include    "sys/file_descriptors.h"
#include    <kernel.h>
#include	"io/keyboard.h"

syscall_fn_t* calls_table[NUM_CALLS] = {0};

/**
 * @brief Обработчик системных вызовов
 * 
 * @param regs - Регистр
 */
void syscall_handler(volatile registers_t regs) {
	if (regs.eax >= NUM_CALLS) {
        qemu_err("Invalid system call: %d!", regs.eax);

        __asm__ volatile("movl %0, %%eax" :: "r"(0));
        return;
    }

    syscall_fn_t* entry_point = (syscall_fn_t*)calls_table[regs.eax];

    if(entry_point == NULL) {
        qemu_err("System call is not defined right now: %d", regs.eax);

        __asm__ volatile("movl %0, %%eax" :: "r"(0));
        return;
    }

    regs.eax = entry_point(regs.ebx, regs.ecx, regs.edx);

    // TODO: Just place result into eax, I know how to do it!

    __asm__ volatile("movl %0, %%eax" :: "r"(regs.eax));
}

size_t syscall_env(struct env* position) {
    memcpy(position, &system_environment, sizeof(env_t));

    return 0;
}

size_t syscall_memory_alloc(size_t size, size_t align, void** out) {
    void* allocated = kcalloc(size, align);

    *out = allocated;

    return 0;
}

size_t syscall_memory_realloc(void* memory, size_t size, void** out) {
	void* r = krealloc(memory, size);

	*out = r;
	
	return 0;
}

size_t syscall_memory_free(void* memory) {
    kfree(memory);

    return 0;
}

size_t syscall_tty_write(const char* text) {
    tty_puts(text);
    return 0;
}

size_t syscall_getkey() {
    return keyboard_buffer_get();
}

size_t syscall_get_timer_ticks() {
    return getTicks();
}

size_t syscall_sleep(uint32_t millis) {
	sleep_ms(millis);
	
    return 0;
}

size_t syscall_datetime(sayori_time_t* out_time) {
	*out_time = get_time();	
	
    return 0;
}

size_t syscall_exit(SAYORI_UNUSED uint32_t status) {
	process_t* proc = get_current_proc();
	
	qemu_log("Exit requested (status %d) by PID %d\n", status, proc->pid);

    if(proc->pid == 0) {
        qemu_warn("Request cancelled because PID == 0");
		return 0;
	}
    
	thread_exit_entrypoint();
    
    return 0;
}

size_t syscall_screen_update() {
  screen_update();

  return 0;
}

size_t syscall_mmap(size_t physical, size_t virtual, size_t size, size_t flags) {
    size_t pagedir = get_current_proc()->page_dir_virt;

    map_pages((void*)pagedir, physical, virtual, size, flags);

    return 1;
}

size_t syscall_munmap(size_t virtual, size_t size) {
    size_t pagedir = get_current_proc()->page_dir_virt;

    unmap_pages_overlapping((void*)pagedir, virtual, size);
    
    return 1;
}

size_t syscall_temperature() {
    if(is_temperature_module_present()) {
        return get_cpu_temperature();
    }

    return 0xFFFFFFFF;
}

size_t syscall_mouse(uint32_t* out_x, uint32_t* out_y, uint32_t* flags) {
    uint32_t x = mouse_get_x();
    uint32_t y = mouse_get_y();

    uint32_t fl = ((uint32_t)mouse_get_b1() << 0) 
    | ((uint32_t)mouse_get_b2() << 1) 
    | ((uint32_t)mouse_get_b3() << 2) 
    | ((uint32_t)mouse_get_b4() << 3) 
    | ((uint32_t)mouse_get_b5() << 4);

    *out_x = x;
    *out_y = y;
    *flags = fl;

    return 0;
}

size_t syscall_getch(uint32_t* out_char) {
    uint32_t ch = getchar();

    *out_char = ch;

    return 0;
}

size_t syscall_tty_flush() {
    tty_update();

    return 0;
}

size_t syscall_tty_write_raw(const char* text, size_t length) {
    tty_puts_raw(text, length);
    return 0;
}

size_t syscall_get_console_size(uint32_t* out_wh) {
    uint32_t w = tty_get_width();
    uint32_t h = tty_get_height();
    qemu_printf("Console rq: %d %d\n", w, h);

  *out_wh = (h << 16) | w;

  return 0;
}

/**
 * @brief Инициализация системных вызовов
 * 
 * @param regs - Регистр
 */
void init_syscalls(void){
	register_interrupt_handler(SYSCALL, &syscall_handler);

	calls_table[0] = (syscall_fn_t *)syscall_env;
    calls_table[1] = (syscall_fn_t *)syscall_mmap;
    calls_table[2] = (syscall_fn_t *)syscall_munmap;
	calls_table[3] = (syscall_fn_t *)syscall_memory_alloc;
	calls_table[4] = (syscall_fn_t *)syscall_memory_realloc;
	calls_table[5] = (syscall_fn_t *)syscall_memory_free;
	calls_table[6] = (syscall_fn_t *)syscall_tty_write;
    calls_table[7] = (syscall_fn_t *)syscall_tty_write_raw;
    calls_table[8] = (syscall_fn_t *)syscall_tty_flush;
    calls_table[9] = (syscall_fn_t *)syscall_getkey;
    calls_table[10] = (syscall_fn_t *)syscall_getch;
    calls_table[11] = (syscall_fn_t *)syscall_mouse;
    calls_table[12] = (syscall_fn_t *)file_descriptor_allocate;
    calls_table[13] = (syscall_fn_t *)file_descriptor_read;
	calls_table[14] = (syscall_fn_t *)file_descriptor_write;
    calls_table[15] = (syscall_fn_t *)file_descriptor_seek;
    calls_table[16] = (syscall_fn_t *)file_descriptor_tell;
    calls_table[17] = (syscall_fn_t *)file_descriptor_close;
    calls_table[18] = (syscall_fn_t *)syscall_datetime;
    calls_table[19] = (syscall_fn_t *)syscall_sleep;
    calls_table[20] = (syscall_fn_t *)syscall_exit;
    calls_table[21] = (syscall_fn_t *)yield;
    calls_table[22] = (syscall_fn_t *)syscall_screen_update;
    calls_table[23] = (syscall_fn_t *)syscall_temperature;
    calls_table[24] = (syscall_fn_t *)syscall_get_timer_ticks;
    calls_table[25] = (syscall_fn_t *)syscall_get_console_size;
    
	qemu_ok("System calls initialized!");
}
