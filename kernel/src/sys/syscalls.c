/**
 * @file sys/syscalls.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY >_ (pikachu_andrey@vk.com)
 * @brief Интерфейс системных вызовов
 * @version 0.4.2
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include	"sys/syscalls.h"
#include	"io/ports.h"
#include    "io/screen.h"
#include	"io/tty.h"
#include	"user/env.h"
#include    "sys/file_descriptors.h"
#include    <kernel.h>
#include	"io/keyboard.h"


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
    return get_cpu_temperature();
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

size_t syscall_get_screen_parameters(size_t screen_id, size_t parameter, uint8_t* out_buffer, size_t length) {
    // TODO: Utilize screen_id somehow.

    if(out_buffer == NULL) {
        return (size_t)-1;
    }

    // Size checking
    if(parameter == SCREEN_QUERY_WIDTH
        || parameter == SCREEN_QUERY_HEIGHT
        || parameter == SCREEN_QUERY_BITS_PER_PIXEL) {
        if(length < 4) {
            return -1;
        }
    } else {
        qemu_err("Parameter #%x doesn't have size checking, quitting early1", parameter);
        return (size_t)-1;
    }

    switch(parameter) {
        case SCREEN_QUERY_WIDTH: 
            *(uint32_t*)out_buffer = getScreenWidth();
            break;
        case SCREEN_QUERY_HEIGHT: 
            *(uint32_t*)out_buffer = getScreenHeight();
            break;
        case SCREEN_QUERY_BITS_PER_PIXEL: 
            *(uint32_t*)out_buffer = getDisplayBpp();
            break;
    }

    return (size_t)-1;
}

size_t syscall_copy_to_screen(size_t screen_id, uint8_t* buffer) {
    // TODO: Utilize screen_id somehow.
    if(buffer == NULL) {
        return (size_t)-1;
    }

    memcpy(getFrameBufferAddr(), buffer, getDisplaySize());

    return 0;
}

size_t syscall_copy_from_screen(size_t screen_id, uint8_t* buffer) {
    // TODO: Utilize screen_id somehow.
    if(buffer == NULL) {
        return (size_t)-1;
    }

    memcpy(buffer, getFrameBufferAddr(), getDisplaySize());

    return 0;
}

syscall_fn_t* calls_table[] = {
    // Environment
	[0] = (syscall_fn_t *)syscall_env,

    // Memory
    [1] = (syscall_fn_t *)syscall_mmap,
    [2] = (syscall_fn_t *)syscall_munmap,
	[3] = (syscall_fn_t *)syscall_memory_alloc,
	[4] = (syscall_fn_t *)syscall_memory_realloc,
	[5] = (syscall_fn_t *)syscall_memory_free,
    
    // TTY
	[6] = (syscall_fn_t *)syscall_tty_write,
    [7] = (syscall_fn_t *)syscall_tty_write_raw,
    [8] = (syscall_fn_t *)syscall_tty_flush,

    // Input
    [9] = (syscall_fn_t *)syscall_getkey,
    [10] = (syscall_fn_t *)syscall_getch,
    [11] = (syscall_fn_t *)syscall_mouse,

    // Files
    [12] = (syscall_fn_t *)file_descriptor_allocate,
    [13] = (syscall_fn_t *)file_descriptor_read,
	[14] = (syscall_fn_t *)file_descriptor_write,
    [15] = (syscall_fn_t *)file_descriptor_seek,
    [16] = (syscall_fn_t *)file_descriptor_tell,
    [17] = (syscall_fn_t *)file_descriptor_close,
    
    // Date & Time
    [18] = (syscall_fn_t *)syscall_datetime,
    [19] = (syscall_fn_t *)syscall_sleep,

    // Control flow
    [20] = (syscall_fn_t *)syscall_exit,
    [21] = (syscall_fn_t *)yield,
    
    // Misc.
    [22] = (syscall_fn_t *)syscall_screen_update,
    [23] = (syscall_fn_t *)syscall_temperature,
    [24] = (syscall_fn_t *)syscall_get_timer_ticks,
    [25] = (syscall_fn_t *)syscall_get_console_size,

    // Screen
    [26] = (syscall_fn_t *)syscall_get_screen_parameters,
    [27] = (syscall_fn_t *)syscall_copy_to_screen,
    [28] = (syscall_fn_t *)syscall_copy_from_screen,
};

#define SYSCALL_COUNT (sizeof(calls_table) / sizeof(syscall_fn_t*))

/**
 * @brief Обработчик системных вызовов
 * 
 * @param regs - Регистр
 */
void syscall_handler(volatile registers_t regs) {
	if (regs.eax >= SYSCALL_COUNT) {
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

    regs.eax = entry_point(regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi);

    // TODO: Just place result into eax, I know how to do it!

    __asm__ volatile("movl %0, %%eax" :: "r"(regs.eax));
}

/**
 * @brief Инициализация системных вызовов
 * 
 * @param regs - Регистр
 */
void init_syscalls(void) {
	register_interrupt_handler(SYSCALL, &syscall_handler);
    
    qemu_log("There are %d system calls available.", SYSCALL_COUNT);

	qemu_ok("System calls initialized!");
}
