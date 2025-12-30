/**
 * @file sys/cpu_isr.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief Обработчик прерываний
 * @version 0.4.3
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2026
 */

#include	"sys/cpu_isr.h"
#include	"sys/unwind.h"
#include <arch/x86/ports.h>
#include    "sys/scheduler.h"
#include 	<io/status_loggers.h>
#include 	<io/logging.h>

_Noreturn void bsod_screen(registers_t* regs, char* title, char* msg, uint32_t code){
    qemu_printf("=== ЯДРО УПАЛО =======================================\n");
    qemu_printf("| \n");
    qemu_printf("| Наименование: %s\n",title);
    qemu_printf("| Код ошибки: %x\n",code);
    qemu_printf("| Сообщение: %s\n",msg);
    qemu_printf("| RAX: %x\n",regs->rax);
    qemu_printf("| RBX: %x\n",regs->rbx);
    qemu_printf("| RCX: %x\n",regs->rcx);
    qemu_printf("| RDX: %x\n",regs->rdx);
    qemu_printf("| RSP: %x\n",regs->rsp);
    qemu_printf("| RBP: %x\n",regs->rbp);
    qemu_printf("| RIP: %x\n",regs->rip);
    qemu_printf("| RFLAGS: %x\n",regs->rflags);
    qemu_printf("| \n");
    qemu_printf("======================================================\n");
    
    #ifdef NOCTURNE_SUPPORT_TIER1
    qemu_err("PROCESS CAUSED THE EXCEPTION: nr. %d", get_current_proc()->pid);
    
    unwind_stack(10);

    if(is_multitask()) {
        qemu_err("PROCESS CAUSED THE EXCEPTION: nr. %d", get_current_proc()->pid);
        
        if(get_current_proc()->pid != 0) {
            qemu_note("EXIT HERE");
            thread_exit_entrypoint();
        }
    }
    #endif

    __asm__ volatile("cli");  // Disable interrupts
    __asm__ volatile("hlt");  // Halt

    while(1) {
        __asm__ volatile("nop");
    }
}

void print_regs(registers_t* regs) {
    qemu_printf("RAX = %x\n", regs->rax);
    qemu_printf("RBX = %x\n", regs->rbx);
    qemu_printf("RCX = %x\n", regs->rcx);
    qemu_printf("RDX = %x\n", regs->rdx);
    qemu_printf("RSP = %x\n", regs->rsp);
    qemu_printf("RBP = %x\n", regs->rbp);
    qemu_printf("RIP = %x\n", regs->rip);
    qemu_printf("RFLAGS = %x\n", regs->rflags);
}

void division_by_zero(registers_t* regs) {
    qemu_log("Exception: DIVISION BY ZERO");
    print_regs(regs);
    bsod_screen(regs,"CRITICAL_ERROR_DZ_DIVISION_BY_ZERO", "Деление на ноль", regs->rax);
}

void fault_opcode(registers_t* regs) {
    qemu_log("FAULT OPERATION CODE...\n");
    print_regs(regs);
    bsod_screen(regs,"CRITICAL_ERROR_UD_FAULT_OPERATION_CODE","Невалидный код",regs->rax);
}

void double_error(registers_t* regs){
    qemu_log("Exception: DOUBLE EXCEPTION\n");
    qemu_log("Error code: %d", regs->err_code);     
    bsod_screen(regs,"CRITICAL_ERROR_DF_DOUBLE_EXCEPTION","Двойное исключение",regs->err_code);
}

void invalid_tss(registers_t* regs){
    uint32_t ext = regs->err_code & EXT_BIT;
    uint32_t idt = regs->err_code & IDT_BIT;
    uint32_t ti = regs->err_code & TI_BIT;
    uint32_t selector = regs->err_code & ERR_CODE_MASK;

    qemu_log("Exception: INVALID TSS\n");
    qemu_log("cause of error: ");

    char* msg = "Недействительный TSS";
    if (ext) {
        qemu_log("HARDWARE INTERRUPT\n");
        msg = "Аппаратное прерывание";
    }

    if (idt) {
        qemu_log("IDT GATE\n");
        msg = "Шлюз IDT";
    }

    if (ti) {
        qemu_log("LDT GATE\n");
        msg = "Шлюз LDT";
    }

    qemu_log("Invalid selector: %d", selector);
    bsod_screen(regs, "CRITICAL_ERROR_TS_INVALID_TS", msg, selector);
}

void segment_is_not_available(registers_t* regs){
    uint32_t ext = regs->err_code & EXT_BIT;
    uint32_t idt = regs->err_code & IDT_BIT;
    uint32_t ti = regs->err_code & TI_BIT;
    uint32_t selector = regs->err_code & ERR_CODE_MASK;

    qemu_log("Exception: SEGMENT IS'T AVAILABLE\n");
    qemu_log("cause of error: ");

    char* msg = "СЕГМЕНТ НЕДОСТУПЕН";
    if (ext)
    {
        qemu_log("HARDWARE INTERRUPT\n");
        msg = "Аппаратное прерывание";
    }

    if (idt)
    {
        qemu_log("IDT GATE\n");
        msg = "Шлюз IDT";
    }

    if (ti)
    {
        qemu_log("LDT GATE\n");
        msg = "Шлюз LDT";
    }

    qemu_log("Invalid selector: %d", selector);
    bsod_screen(regs,"CRITICAL_ERROR_NP_SEGMENT_IST_AVAILABLE", msg, selector);
}

void stack_error(registers_t* regs){
    qemu_log("Exception: STACK ERROR\n");
    qemu_log("Error code: %d ", regs->err_code);
    bsod_screen(regs,"CRITICAL_ERROR_SS_STACK_ERROR","Ошибка стека",regs->err_code);
}

void general_protection_error(registers_t* regs) {
    qemu_log("Exception: GENERAL PROTECTION ERROR\n");
    qemu_log("Error code: %d", regs->err_code);

    bsod_screen(regs,"CRITICAL_ERROR_GP_GENERAL_PROTECTION", "Общая ошибка защиты", regs->err_code);
}

void page_fault(registers_t* regs){
    uint32_t fault_addr = read_cr2();
    int present = !(regs->err_code & 0x1);       /* Page not present */
    uint32_t rw = regs->err_code & 0x2;          /* Page is read only */
    uint32_t user = regs->err_code & 0x4;        /* User mode */
    uint32_t reserved = regs->err_code & 0x8;    /* Reserved bits is wrote */
    uint32_t id = regs->err_code & 0x10;         /* Instruction fetch */

    qemu_log("Page fault: ");
    char* msg = "Переполнение памяти буфера";
    
    if (present){
        qemu_log("NOT PRESENT, ");
        msg = "Память по данному адресу недоступна";
    }
    if (rw){
        qemu_log("READ ONLY, ");
        msg = "Память только для чтения";
    }
    if (user){
        qemu_log("USER MODE,  ");
        msg = "Память только для пользователя";
    }
    if (reserved){
        qemu_log("WRITING TO RESERVED BITS, ");
        msg = "Попытка записи в зарез. биты";
    }
    if (id){
        qemu_log("EIP error ");
        msg = "Ошибка EIP";
    }
    qemu_log("at address (virtual) %x",fault_addr);

    #ifdef NOCTURNE_SUPPORT_TIER1
    if(tty_is_initialized()) {
        tty_printf("Process nr.%d caused page fault: ", get_current_proc()->pid); 
        if (present){
            tty_printf("NOT PRESENT, ");
        }
        if (rw){
            tty_printf("READ ONLY, ");
        }
        if (user){
            tty_printf("USER MODE,  ");
        }
        if (reserved){
            tty_printf("WRITING TO RESERVED BITS, ");
        }
        if (id){
            tty_printf("EIP error ");
        }

        tty_printf("at address (virtual) %x",fault_addr);
    }

    #endif

    // Prevent kmallocs (in bsod_screen()) in Page Fault
    bsod_screen(regs, "CRITICAL_ERROR_PF_PAGE_FAULT", msg, fault_addr);
}

void fpu_fault(registers_t* regs){
    qemu_log("Exception: FPU_FAULT\n");
    qemu_log("Error code: %d ", regs->err_code);
    bsod_screen(regs, "CRITICAL_ERROR_FPU_FAULT", "Ошибка FPU", regs->err_code);
}
