#include "arch/x86/tss.h"
#include "arch/x86/gdt.h"
#include "lib/string.h"
#include "io/logging.h"

tss_entry_t tss;

void write_tss(int32_t num, uint32_t ss0, uint32_t esp0){
    /* очищaем структуру tss */
    memset(&tss, 0, sizeof(tss_entry_t));
    /* Селектор сегмента стека (Stack Segment Selector)
       для уровня привилегий 0 (кольцо 0). */
    qemu_log("TSS: SELECTOR #%d; SS0: %08x; ESP0: %08x", num, ss0, esp0);

    tss.ss0 = ss0;
    /* Указатель стека для уровня привилегий 0 */
    tss.esp0 = esp0;
   //  tss.iomap_base = sizeof(tss_entry_t);

    tss.cs = 0x0b;  // Kernel code segment but orred by 3
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13;   // Kernel data segment but orred by 3

    // GDT entry is OK!
    /* база tss */
    uint32_t base = (uint32_t) &tss;
    /* limit tss */
    uint32_t limit = sizeof(tss);
    /* Заполняем дескриптор TSS в GDT:
       Создаем указатель на соответствующую запись в GDT для доступа и
       инициализации отдельных полей дескриптора TSS */
    gdt_entry_t* tss_d = (gdt_entry_t*) &gdt_entries[num];
    /* Устанавливаем базу и лимит */
    tss_d->base_low = base & 0xFFFF;
    tss_d->base_middle = (base >> 16) & 0xFF;
    tss_d->base_high = (base >> 24) & 0xFF;
    tss_d->limit_low = limit & 0xFFFF;

    tss_d->granularity = ((limit >> 16) & 0xF) | (0 << 4);
    tss_d->access = 0x89;
}
