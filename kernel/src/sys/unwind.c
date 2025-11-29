#include "sys/unwind.h"
#include "io/ports.h"
#include "io/tty.h"
#include "lib/string.h"
#include "io/logging.h"

volatile size_t NOCTURNE_ksym_data_start = 0;
volatile size_t NOCTURNE_ksym_data_end = 0;

#ifndef RELEASE

char _temp_funcname[512] = {0};

// Returns true if okay, function name stored in _temp_funcname
bool get_func_name_by_addr(size_t addr) {
    char* temp = (char*)NOCTURNE_ksym_data_start;

    if(temp == 0) {
        // qemu_err("ksym was not initialized before.");
        return false;
    }

    do {    
        uint32_t current_addr = *(uint32_t*)temp;
        
        temp += 4; // Address

        size_t namelen = (size_t)*(uint8_t*)temp;

        temp += 1; // Name length

        memcpy(_temp_funcname, temp, namelen);
        _temp_funcname[namelen] = 0;

        temp += namelen;

        uint32_t next_addr = *(uint32_t*)temp;
        
        if(addr >= current_addr && addr < next_addr) {
            return true;
        }
    } while((size_t)temp < NOCTURNE_ksym_data_end);

    return false;
}

void unwind_stack(uint32_t max_frames) {
    qemu_log("Unwind!");
    
    struct stackframe *stk = 0;

    #ifdef NOCTURNE_X86
    __asm__ volatile("movl %%ebp, %0" : "=r"(stk) :: );
    #else
    __asm__ volatile("mov %%rbp, %0" : "=r"(stk) :: );
    #endif

    qemu_printf("Stack trace:\n");

    if(tty_is_initialized()) {
        tty_printf("Stack trace:\n");
    }

    for(uint32_t frame = 0; stk && frame < max_frames; ++frame) {
        bool exists = get_func_name_by_addr(stk->eip);

        qemu_printf("  Frame #%d => %x  ->   ", frame, stk->eip);

        if(tty_is_initialized()) {
            tty_printf("  Frame #%d => %x  ->   ", frame, stk->eip);
        }

        qemu_printf("%s\n", exists ? _temp_funcname : "???");

        if(tty_is_initialized()) {
            tty_printf("%s\n", exists ? _temp_funcname : "???");
        }

        stk = stk->ebp;
    }
}
#else

void unwind_stack(uint32_t max_frames) {
    (void)max_frames;
}

#endif
