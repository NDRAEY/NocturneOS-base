#pragma once

#include	"common.h"
#include    "sys/registers.h"
#include	"lib/list.h"
#include	"mem/pmm.h"
#include	"elf/elf.h"

#define DEFAULT_STACK_SIZE (64 << 10)

typedef enum {
    CREATED = 0,
    RUNNING,
    PAUSED,
    DEAD
} thread_state_t;

SAYORI_INLINE const char* thread_state_string(thread_state_t state) {
    switch (state) {
        case CREATED:
            return "CREATED";
        case RUNNING:
            return "RUNNING";
        case PAUSED:
            return "PAUSED";
        case DEAD:
            return "DEAD";
        default:
            return "UNKNOWN";
    }
}

typedef	volatile struct {
    // 0
	list_item_t		list_item;		/* List item */
	// 12
    physical_addr_t	page_dir;		/* Page directory */
	// 16
    size_t			threads_count;	/* Count of threads */
    // 20
	uint32_t		pid;		/* Process ID (PID) */
    // 24
    virtual_addr_t  page_dir_virt;	/* Virtual address of page directory */
    // 28
	char*			name;		/* Process name */
	// 32
	size_t          page_tables_virts[1024];    /* Page table addresses */
    // Every process should have a path that process operates
    char*           cwd;
    // If process is a program, it should contain elf header.
    elf_t*          program;
} process_t;

/*-----------------------------------------------------------------------------
 * 		Thread structure
 *---------------------------------------------------------------------------*/
typedef volatile struct
{
    // 0
	list_item_t		list_item;			/* List item */
    // 12
	process_t*		process;			/* This thread's process */
    // 16
	uint32_t		flags;			/* Flags*/
    // 20
	size_t			stack_size;			/* Size of thread's stack */
    // 24
	void*			stack;
    // 28
	uint32_t		esp;				/* Thread state */
    // 32
	uint32_t		entry_point;
    // 36
	uint32_t		id;				/* Thread ID */
    // 40
	uint32_t		stack_top;
    // 72
    thread_state_t state;
} thread_t;

/* Initialization */
void init_task_manager(void);

void task_switch_v2_wrapper(SAYORI_UNUSED registers_t regs);
extern void task_switch_v2(thread_t*, thread_t*);

thread_t* _thread_create_unwrapped(process_t* proc, void* entry_point, size_t stack_size,
                                   bool kernel);

/* Create new thread */
thread_t* thread_create(process_t* proc,
	               	    void* entry_point,
	               	    size_t stack_size,
	               	    bool kernel);
                        
thread_t* thread_create_arg1(process_t* proc, void* entry_point, size_t stack_size,
bool kernel, size_t arg1);

/* Get current process */
volatile process_t* get_current_proc(void);

/* Exit from thread */
void thread_exit(thread_t* thread);

size_t create_process(void* entry_point, char* name, bool is_kernel);

/* Check multitask flag */
bool is_multitask(void);

/* Switch to user mode */
extern void user_mode_switch(void* entry_point, uint32_t user_stack_top);

/* Init user mode */
void init_user_mode(void* entry_point, size_t stack_size);

void scheduler_mode(bool on);

SAYORI_INLINE void yield() {
    task_switch_v2_wrapper((registers_t){});
}

bool process_exists(size_t pid);
void process_wait(size_t pid);

void idle_thread(void);

__attribute__((noreturn)) void thread_exit_entrypoint();

int32_t spawn_prog(const char *name, int argc, const char* const* eargv);

void process_add_prepared(volatile process_t* process);
void thread_add_prepared(volatile thread_t* thread);

void process_remove_prepared(volatile process_t* process);
void thread_remove_prepared(volatile thread_t* thread);