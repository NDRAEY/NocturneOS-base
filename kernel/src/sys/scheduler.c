/**
 * @file sys/scheduler.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru), NDRAEY (pikachu_andrey@vk.com)
 * @brief Менеджер задач
 * @version 0.4.2
 * @date 2025-04-13
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */

#include <sys/scheduler.h>
#include <lib/string.h>
#include <io/logging.h>
#include "mem/vmm.h"
#include "lib/math.h"
#include "sys/sync.h"

list_t process_list;
list_t thread_list;				

uint32_t next_pid = 0;			
uint32_t next_thread_id = 0;	
bool multi_task = false;		
process_t* kernel_proc = 0;		
thread_t* kernel_thread = 0;	
process_t* current_proc = 0;	
thread_t* current_thread = 0;

extern uint32_t __init_esp;

bool scheduler_working = true;

extern physical_addr_t kernel_page_directory;

mutex_t proclist_scheduler_mutex = {.lock = false};
mutex_t threadlist_scheduler_mutex = {.lock = false};

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
	kernel_proc->cwd = strdynamize("rd0:/");
	
	list_add(&process_list, (void*)&kernel_proc->list_item);

	/* Create kernel thread */
	kernel_thread = (thread_t*) kmalloc(sizeof(thread_t));

	memset((void*)kernel_thread, 0, sizeof(thread_t));

	kernel_thread->process = kernel_proc;
	kernel_thread->list_item.list = nullptr;
	kernel_thread->id = next_thread_id++;
	kernel_thread->stack_size = DEFAULT_STACK_SIZE;
	kernel_thread->esp = esp;
	kernel_thread->stack_top = __init_esp;

	list_add((void*)&thread_list, (void*)&kernel_thread->list_item);

	current_proc = kernel_proc;
	current_thread = kernel_thread;

	__asm__ volatile ("sti");

	/* Enable multitasking flag */
	multi_task = true;

    //thread_create(kernel_proc, idle_thread, DEFAULT_STACK_SIZE, false);

    qemu_ok("OK");
}

void scheduler_mode(bool on) {
	scheduler_working = on;
}

size_t create_process(void* entry_point, char* name, bool is_kernel) {
    process_t* proc = allocate_one(process_t);

	proc->pid = next_pid++;
	proc->list_item.list = nullptr;  // No nested processes
	proc->threads_count = 0;

	proc->name = strdynamize(name);

    // Inherit path
	proc->cwd = strdynamize(get_current_proc()->cwd);
    
    process_add_prepared(proc);

    thread_t* thread = _thread_create_unwrapped(proc, entry_point, DEFAULT_STACK_SIZE, is_kernel);

    qemu_log("PID: %d, DIR: %x; Threads: %d", proc->pid, proc->page_dir, proc->threads_count);

	thread_add_prepared(thread);

    void* virt = clone_kernel_page_directory((size_t*)proc->page_tables_virts);
    uint32_t phys = virt2phys(get_kernel_page_directory(), (virtual_addr_t) virt);

    proc->page_dir = phys;
    proc->page_dir_virt = (size_t)virt;

    qemu_log("FINISHED!");

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
 *
 * @return thread_t* - Поток
 */
thread_t* _thread_create_unwrapped(process_t* proc, void* entry_point, size_t stack_size,
                                   bool kernel) {
    (void)kernel;

    void*	stack = nullptr;
    uint32_t	eflags;

    qemu_log("Process at: %p", proc);
    qemu_log("Stack size: %d", stack_size);
    qemu_log("Entry point: %p", entry_point);
    qemu_log("Kernel: %d", kernel);

    /* Create new thread handler */
    thread_t* tmp_thread = (thread_t*) kcalloc(sizeof(thread_t), 1);

    /* Initialization of thread  */
    tmp_thread->id = next_thread_id++;
    tmp_thread->list_item.list = nullptr;
    tmp_thread->process = proc;
    tmp_thread->stack_size = stack_size;
    tmp_thread->entry_point = (uint32_t) entry_point;

    /* Create thread's stack */
    stack = kmalloc_common(stack_size, 16);
    memset(stack, 0, stack_size);

    tmp_thread->stack = stack;
    tmp_thread->esp = (uint32_t) stack + stack_size - (7 * 4);
    tmp_thread->stack_top = (uint32_t) stack + stack_size;

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

    tmp_thread->state = PAUSED;

    /* Add thread to ring queue */
    thread_add_prepared(tmp_thread);

    return tmp_thread;
}

thread_t* _thread_create_unwrapped_arg1(process_t* proc, void* entry_point, size_t stack_size, bool kernel, size_t arg1) {
    (void)kernel;

    void*	stack = nullptr;
    uint32_t	eflags;

    qemu_log("Process at: %p", proc);
    qemu_log("Stack size: %d", stack_size);
    qemu_log("Entry point: %p", entry_point);
    qemu_log("Kernel: %d", kernel);

    /* Create new thread handler */
    thread_t* tmp_thread = (thread_t*) kcalloc(sizeof(thread_t), 1);

    /* Initialization of thread  */
    tmp_thread->id = next_thread_id++;
    tmp_thread->list_item.list = nullptr;
    tmp_thread->process = proc;
    tmp_thread->stack_size = stack_size;
    tmp_thread->entry_point = (uint32_t) entry_point;

    /* Create thread's stack */
    stack = kmalloc_common(stack_size, 16);
    memset(stack, 0, stack_size);

    tmp_thread->stack = stack;
    tmp_thread->esp = (uint32_t) stack + stack_size - (8 * 4);
    tmp_thread->stack_top = (uint32_t) stack + stack_size;

    /* Add thread to ring queue */
    thread_add_prepared(tmp_thread);

    /* Thread's count increment */
    proc->threads_count++;

    /* Fill stack */

    /* Create pointer to stack frame */
    uint32_t* esp = (uint32_t*) ((char*)stack + stack_size);

    // Get EFL
    __asm__ volatile ("pushf; pop %0" :"=r"(eflags)::"memory");

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
    bool kernel) {
    if(!multi_task) {
        qemu_err("Scheduler is disabled!");
        return NULL;
    }

    /* Disable all interrupts */
    __asm__ volatile ("cli");

    /* Create new thread handler */
    thread_t* tmp_thread = (thread_t*) _thread_create_unwrapped(proc, entry_point, stack_size, kernel);

    tmp_thread->state = CREATED;

    /* Enable all interrupts */
    __asm__ volatile ("sti");

    qemu_ok("CREATED THREAD");

    return tmp_thread;
}

thread_t* thread_create_arg1(process_t* proc, void* entry_point, size_t stack_size,
    bool kernel, size_t arg1) {
    if(!multi_task) {
        qemu_err("Scheduler is disabled!");
        return NULL;
    }

    __asm__ volatile ("cli");

    thread_t* tmp_thread = (thread_t*) _thread_create_unwrapped_arg1(proc, entry_point, stack_size, kernel, arg1);

    tmp_thread->state = CREATED;

    __asm__ volatile ("sti");

    qemu_ok("CREATED THREAD");

    return tmp_thread;
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
        __asm__ volatile("nop" ::: "memory");
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

            thread_remove_prepared((thread_t*)next_thread);

            qemu_log("REMOVED FROM LIST");

            kfree(next_thread->stack);
            kfree((void*)next_thread);

            qemu_log("FREED MEMORY");

            process->threads_count--;

			bool is_kernels_pid = current_proc->pid == 0;
            // NOTE: We should be in kernel process (PID 0) to free page tables and process itself.
            // TODO: Switch to kernel's PD here, because process info stored there
            if(process->threads_count == 0 && is_kernels_pid) {
                qemu_warn("PROCESS #%d `%s` DOES NOT HAVE ANY THREADS", process->pid, process->name);

                if(process->program) {
                    for (int32_t i = 0; i < process->program->elf_header.e_phnum; i++) {
                        Elf32_Phdr *phdr = process->program->p_header + i;

                        if(phdr->p_type != PT_LOAD)
                            continue;

                        size_t pagecount = MAX((ALIGN(phdr->p_memsz, PAGE_SIZE) / PAGE_SIZE), 1U);

                        for(size_t x = 0; x < pagecount; x++) {
                            size_t vaddr = phdr->p_vaddr + (x * PAGE_SIZE);
                            size_t paddr = virt2phys_ext((void*)process->page_dir_virt, process->page_tables_virts, vaddr);

                            qemu_log("Page dir: %x; Free: %x -> %x", process->page_dir_virt, vaddr, paddr);

                            phys_free_single_page(paddr);
                        }
                    }

                    unload_elf(process->program);

                    qemu_log("Program unloaded.");
                }

                for(size_t pt = 0; pt < 1023; pt++) {
                    size_t page_table = process->page_tables_virts[pt];
                    if(page_table) {
                        qemu_note("[%-4d] <%08x - %08x> USED PAGE TABLE AT: %x", 
                            pt,
                            (pt * PAGE_SIZE) << 10,
                            ((pt + 1) * PAGE_SIZE) << 10,
                            page_table
                        );
                        kfree((void *) page_table);
                    }
                }

                qemu_log("FREED PAGE TABLES");

                kfree((void *) process->page_dir_virt);

                qemu_log("FREED PAGE DIR");
                
                kfree(process->name);
                kfree(process->cwd);

                process_remove_prepared((process_t*)process);

                qemu_log("REMOVED PROCESS FROM LIST");
                kfree((void*)process);

                qemu_log("FREED PROCESS LIST ITEM");
            }
        }

        next_thread = next_thread_soon;
    }

    task_switch_v2(current_thread, next_thread);
}

void idle_thread(void) {
    while(1) {
        __asm__ volatile("hlt" ::: "memory");
        yield();
    }
}

void process_add_prepared(volatile process_t* process) {
    mutex_get(&proclist_scheduler_mutex);

    list_add(&process_list, (list_item_t*)&process->list_item);

    mutex_release(&proclist_scheduler_mutex);
}

void thread_add_prepared(volatile thread_t* thread) {
    mutex_get(&threadlist_scheduler_mutex);

    list_add(&thread_list, (list_item_t*)&thread->list_item);

    mutex_release(&threadlist_scheduler_mutex);
}

void process_remove_prepared(volatile process_t* process) {
    mutex_get(&proclist_scheduler_mutex);

    list_remove(&process->list_item);

    mutex_release(&proclist_scheduler_mutex);
}

void thread_remove_prepared(volatile thread_t* thread) {
    mutex_get(&threadlist_scheduler_mutex);

    list_remove(&thread->list_item);

    mutex_release(&threadlist_scheduler_mutex);
}