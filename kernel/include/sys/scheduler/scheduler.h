#pragma once

#include "sys/scheduler/thread.h"

#ifdef NOCTURNE_X86
#include "arch/x86/registers.h"
#endif

#ifdef NOCTURNE_X86_64
#include "arch/x86_64/registers64.h"
#endif


// 128 KB stack for each user thread
#define DEFAULT_STACK_SIZE (128 << 10)

/* Initialization */
void init_task_manager(void);

void task_switch_v2_wrapper(registers_t* regs);
extern void task_switch_v2(thread_t*, thread_t*);

size_t create_process(void* entry_point, char* name, bool is_kernel);

/* Check multitask flag */
bool is_multitask(void);

/* Switch to user mode */
extern void user_mode_switch(void* entry_point, uint32_t user_stack_top);

/* Init user mode */
void init_user_mode(void* entry_point, size_t stack_size);

void scheduler_mode(bool on);

void yield();

bool process_exists(size_t pid);
void process_wait(size_t pid);

void idle_thread(void);

__attribute__((noreturn)) void thread_exit_entrypoint();

int32_t spawn_prog(const char *name, int argc, const char* const* eargv);

void process_add_prepared(process_t* process);
void thread_add_prepared(thread_t* thread);

void process_remove_prepared(process_t* process);
void thread_remove_prepared(thread_t* thread);

void enter_usermode(void (*ep)());
