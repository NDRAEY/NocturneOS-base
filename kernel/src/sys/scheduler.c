/**
 * @file sys/scheduler.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY (pikachu_andrey@vk.com)
 * @brief Менеджер задач
 * @version 0.3.5
 * @date 2025-04-13
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */
#include	"sys/scheduler.h"
#include	"lib/string.h"
#include	"io/ports.h"
#include "mem/vmm.h"

list_t process_list;
list_t thread_list;				

uint32_t next_pid = 0;			
uint32_t next_thread_id = 0;	
bool multi_task = false;		
process_t* kernel_proc = 0;		
thread_t* kernel_thread = 0;	
process_t* current_proc = 0;	
thread_t* current_thread = 0;	
extern uint32_t init_esp;

bool scheduler_working = true;

extern physical_addr_t kernel_page_directory;

/**
 * @brief Initializes scheduler
 */
void init_task_manager(void){
	uint32_t esp = 0;
	__asm__ volatile("mov %%esp, %0" : "=a"(esp));

	/* Disable all interrupts */
	__asm__ volatile ("cli");

	list_init(&process_list);
	list_init(&thread_list);

	/* Create kernel process */
	kernel_proc = (process_t*)kcalloc(sizeof(process_t), 1);

	kernel_proc->pid = next_pid++;
    // NOTE: Page directory address must be PHYSICAL!
	kernel_proc->page_dir = kernel_page_directory;
	kernel_proc->list_item.list = nullptr;
	kernel_proc->threads_count = 1;

	kernel_proc->name = strdynamize("kernel");
	kernel_proc->cwd = strdynamize("R:/");
	
    kernel_proc->suspend = false;

	list_add(&process_list, (void*)&kernel_proc->list_item);

	/* Create kernel thread */
	kernel_thread = (thread_t*) kmalloc(sizeof(thread_t));

	memset((void*)kernel_thread, 0, sizeof(thread_t));

	kernel_thread->process = kernel_proc;
	kernel_thread->list_item.list = nullptr;
	kernel_thread->id = next_thread_id++;
	kernel_thread->stack_size = DEFAULT_STACK_SIZE;
	kernel_thread->suspend = false;
	kernel_thread->esp = esp;
	kernel_thread->stack_top = init_esp;

	list_add((void*)&thread_list, (void*)&kernel_thread->list_item);

	current_proc = kernel_proc;
	current_thread = kernel_thread;

	__asm__ volatile ("sti");

	/* Enable multitasking flag */
	multi_task = true;

    qemu_ok("OK");
}

void scheduler_mode(bool on) {
	scheduler_working = on;
}

size_t create_process(void* entry_point, char* name, bool suspend, bool is_kernel) {
    scheduler_working = false;
	__asm__ volatile("cli");

    process_t* proc = (process_t*)kcalloc(1, sizeof(process_t));

	proc->pid = next_pid++;
	proc->list_item.list = nullptr;  // No nested processes hehe :)
	proc->threads_count = 0;

	proc->name = strdynamize(name);

    // Inherit path
	proc->cwd = strdynamize(get_current_proc()->cwd);
    
	proc->suspend = suspend;

    list_add(&process_list, (void*)&proc->list_item);

    thread_t* thread = _thread_create_unwrapped(proc, entry_point, DEFAULT_STACK_SIZE, is_kernel, suspend);

    qemu_log("PID: %d, DIR: %x; Threads: %d; Suspend: %d", proc->pid, proc->page_dir, proc->threads_count, proc->suspend);

	list_add(&thread_list, (void*)&thread->list_item);

    void* virt = clone_kernel_page_directory((size_t*)proc->page_tables_virts);
    uint32_t phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t) virt);

    proc->page_dir = phys;
    proc->page_dir_virt = (size_t)virt;

    qemu_log("FINISHED!");

	__asm__ volatile("sti");
    scheduler_working = true;

    return proc->pid;
}

 /**
 * @brief Get current process
 *
 * @return process_t* - Current process
 */
 volatile process_t * get_current_proc(void) {
    return current_proc;
}

__attribute__((noreturn)) void thread_exit_entrypoint() {
    qemu_note("THREAD %d WANTS TO EXIT!", current_thread->id);
    thread_exit(current_thread);
    while(1)  // If something goes wrong, we loop here.
        ;
}

/**
 * @brief Создание потока
 * 
 * @param proc - Процесс
 * @param entry_point - Точка входа
 * @param stack_size - Размер стека
 * @param kernel - Функция ядра?
 * @param suspend - Остановлено?
 *
 * @return thread_t* - Поток
 */
thread_t* _thread_create_unwrapped(process_t* proc, void* entry_point, size_t stack_size,
                                   bool kernel, bool suspend) {
    (void)kernel;

    void*	stack = nullptr;
    uint32_t	eflags;

    qemu_log("Process at: %x", proc);
    qemu_log("Stack size: %d", stack_size);
    qemu_log("Entry point: %x", entry_point);
    qemu_log("Suspend: %d", suspend);
    qemu_log("Kernel: %d", kernel);

    /* Create new thread handler */
    thread_t* tmp_thread = (thread_t*) kcalloc(sizeof(thread_t), 1);

    /* Initialization of thread  */
    tmp_thread->id = next_thread_id++;
    tmp_thread->list_item.list = nullptr;
    tmp_thread->process = proc;
    tmp_thread->stack_size = stack_size;
    tmp_thread->suspend = suspend;/* */
    tmp_thread->entry_point = (uint32_t) entry_point;

    /* Create thread's stack */
    stack = kmalloc_common(stack_size, 16);
    memset(stack, 0, stack_size);

    tmp_thread->stack = stack;
    tmp_thread->esp = (uint32_t) stack + stack_size - (7 * 4);
    tmp_thread->stack_top = (uint32_t) stack + stack_size;

    /* Add thread to ring queue */
    list_add(&thread_list, (void*)&tmp_thread->list_item);

    /* Thread's count increment */
    proc->threads_count++;

    /* Fill stack */

    /* Create pointer to stack frame */
    uint32_t* esp = (uint32_t*) ((char*)stack + stack_size);

    // Get EFL
    __asm__ volatile ("pushf; pop %0":"=r"(eflags));

    eflags |= (1 << 9);

    // Fill the stack.
    // On normal systems (like in Linux) exit is called manually, but if something goes wrong, give this task a peaceful death.
    esp[-1] = (uint32_t) thread_exit_entrypoint;
    esp[-2] = (uint32_t) entry_point;
    esp[-3] = eflags;

    // Those are EBX, ESI, EDI and EBP
    esp[-4] = 0;
    esp[-5] = 0;
    esp[-6] = 0;
    esp[-7] = 0;

    return tmp_thread;
}

thread_t* _thread_create_unwrapped_arg1(process_t* proc, void* entry_point, size_t stack_size, bool kernel, bool suspend, size_t arg1) {
    (void)kernel;

    void*	stack = nullptr;
    uint32_t	eflags;

    qemu_log("Process at: %x", proc);
    qemu_log("Stack size: %d", stack_size);
    qemu_log("Entry point: %x", entry_point);
    qemu_log("Suspend: %d", suspend);
    qemu_log("Kernel: %d", kernel);

    /* Create new thread handler */
    thread_t* tmp_thread = (thread_t*) kcalloc(sizeof(thread_t), 1);

    /* Initialization of thread  */
    tmp_thread->id = next_thread_id++;
    tmp_thread->list_item.list = nullptr;
    tmp_thread->process = proc;
    tmp_thread->stack_size = stack_size;
    tmp_thread->suspend = suspend;/* */
    tmp_thread->entry_point = (uint32_t) entry_point;

    /* Create thread's stack */
    stack = kmalloc_common(stack_size, 16);
    memset(stack, 0, stack_size);

    tmp_thread->stack = stack;
    tmp_thread->esp = (uint32_t) stack + stack_size - (8 * 4);
    tmp_thread->stack_top = (uint32_t) stack + stack_size;

    /* Add thread to ring queue */
    list_add(&thread_list, (void*)&tmp_thread->list_item);

    /* Thread's count increment */
    proc->threads_count++;

    /* Fill stack */

    /* Create pointer to stack frame */
    uint32_t* esp = (uint32_t*) ((char*)stack + stack_size);

    // Get EFL
    __asm__ volatile ("pushf; pop %0":"=r"(eflags));

    eflags |= (1 << 9);

    // Fill the stack.
    // On normal systems (like in Linux) exit is called manually, but if something goes wrong, give this task a peaceful death.
    esp[-1] = arg1;
    esp[-2] = (uint32_t) thread_exit_entrypoint;
    esp[-3] = (uint32_t) entry_point;
    esp[-4] = eflags;

    // Those are EBX, ESI, EDI and EBP
    esp[-5] = 0;
    esp[-6] = 0;
    esp[-7] = 0;
    esp[-8] = 0;


    return tmp_thread;
}

thread_t* thread_create(process_t* proc, void* entry_point, size_t stack_size,
    bool kernel, bool suspend) {
    if(!multi_task) {
        qemu_err("Scheduler is disabled!");
        return NULL;
    }

    /* Disable all interrupts */
    __asm__ volatile ("cli");

    /* Create new thread handler */
    thread_t* tmp_thread = (thread_t*) _thread_create_unwrapped(proc, entry_point, stack_size, kernel, suspend);

    /* Enable all interrupts */
    __asm__ volatile ("sti");

    qemu_ok("CREATED THREAD");

    return tmp_thread;
}

thread_t* thread_create_arg1(process_t* proc, void* entry_point, size_t stack_size,
    bool kernel, bool suspend, size_t arg1) {
    if(!multi_task) {
        qemu_err("Scheduler is disabled!");
        return NULL;
    }

    __asm__ volatile ("cli");

    thread_t* tmp_thread = (thread_t*) _thread_create_unwrapped_arg1(proc, entry_point, stack_size, kernel, suspend, arg1);

    __asm__ volatile ("sti");

    qemu_ok("CREATED THREAD");

    return tmp_thread;
}

void thread_suspend(thread_t* thread, bool suspend) {
	thread->suspend = suspend;
}

void thread_exit(thread_t* thread){
	if(!multi_task) {
        qemu_err("Scheduler is disabled!");
        return;
    }

    /* Disable all interrupts */
	__asm__ volatile ("cli");

	/* Mark it as deas */
    thread->state = DEAD;

	/* Load to ECX switch function address */
	__asm__ volatile ("mov %0, %%ecx"::"a"(&task_switch_v2_wrapper));

	/* Enable all interrupts */
	__asm__ volatile ("sti");

	/* Jump to switch_task() */
	__asm__ volatile ("call *%ecx");
}

bool process_exists(size_t pid) {
    process_t* proc = get_current_proc();

    do {
        if(proc->pid == pid) {
            return true;
        }

        proc = (volatile process_t*)proc->list_item.next;
    } while(proc != NULL && proc->pid != 0);

    return false;
}

void process_wait(size_t pid) {
    while(process_exists(pid)) {
        __asm__ volatile("nop");
        yield();
    }
}

bool is_multitask(void){
    return multi_task;
}

void task_switch_v2_wrapper(SAYORI_UNUSED registers_t regs) {
    if(!multi_task) {
        // qemu_err("Scheduler is disabled!");
        return;
    }

    thread_t* next_thread = (thread_t *)current_thread->list_item.next;

    while(next_thread != NULL && (next_thread->state == PAUSED || next_thread->state == DEAD)) {
        thread_t* next_thread_soon = (thread_t *)next_thread->list_item.next;

        if(next_thread->state == DEAD) {
        	qemu_log("QUICK NOTICE: WE ARE IN PROCESS NR. #%u", current_proc->pid);
        	
            process_t* process = next_thread->process;
            qemu_log("REMOVING DEAD THREAD: #%u", next_thread->id);

            list_remove((void*)&next_thread->list_item);

            qemu_log("REMOVED FROM LIST");

            kfree(next_thread->stack);
            kfree((void*)next_thread);

            qemu_log("FREED MEMORY");

            process->threads_count--;

            qemu_log("MODIFIED PROCESS");

			bool is_krnl_process = current_proc->pid == 0;
            // TODO: Switch to kernel's PD here, because process info stored there
            if(process->threads_count == 0 && is_krnl_process)  {
                qemu_log("PROCESS #%d `%s` DOES NOT HAVE ANY THREADS", process->pid, process->name);

                for(size_t pt = 0; pt < 1024; pt++) {
                    size_t page_table = process->page_tables_virts[pt];
                    if(page_table) {
                        qemu_note("[%d] FREE PAGE TABLE AT: %x", pt, page_table);
                        kfree((void *) page_table);
                    }
                }

                qemu_log("FREED PAGE TABLES");

                kfree((void *) process->page_dir_virt);

                qemu_log("FREED SPACE FOR TABLES");

                list_remove((void*)&process->list_item);

                qemu_log("REMOVED PROCESS FROM LIST");

                kfree(process->name);
                kfree((void*)process);

                qemu_log("FREED PROCESS LIST ITEM");
            }
        }

        next_thread = next_thread_soon;
    }

    task_switch_v2(current_thread, next_thread);
}
