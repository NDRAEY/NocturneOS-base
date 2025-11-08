/**
 * @file sys/gdt.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief (GDT) Глобальная таблица дескрипторов
 * @version 0.4.2
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2025
 */
 #include  "arch/x86/gdt.h"
 #include  "arch/x86/tss.h"
#include  "lib/string.h"
#include  <arch/x86/ports.h>

extern uint32_t __init_esp;

tss_entry_t tss;

extern void gdt_flush(uint32_t);
extern void tss_flush(uint32_t tr_selector);

gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran){
    /* Извлекаем нижнюю часть базы адреса (биты 0-15) */
    gdt_entries[num].base_low = (base & 0xFFFF);
    /* Извлекаем среднюю часть базы адреса (биты 16-23) */
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    /* Извлекаем верхнюю часть базы (биты 24-31) */
    gdt_entries[num].base_high = (base >> 24) & 0xFF;
    /* Извлекаем нижнюю часть лимита (биты 0-15) */
    gdt_entries[num].limit_low = (limit & 0xFFFF);
    /* Granulary - это байт, который мы получаем,
       сдвинув limit на два байта вправо, при этом
       верхний полубайт будет содержать флаги (мы
       учитываем это, когда формируем передавемый
       в параметрах limit). На первом этапе мы
       получам верхнюю часть limit */
    gdt_entries[num].granularity = (limit >> 16) & 0xF;
    /* Тепер полученные биты объединяем с верхним
       полубайтом gran. С текущими передаваемыми
       параметрами, мы приходим к тому, что
       устанавливаются флаги G и D/B, что дает нам
       размер страницы в четырехкилобайтовых единицах
       и 32-двух разрядные смещения при доступе к ней */
    gdt_entries[num].granularity |= gran & 0xF0;
    /* Access переписываем без изменений */
    gdt_entries[num].access = access;
}


void write_tss(int32_t num, uint32_t ss0, uint32_t esp0){
    /* очищaем структуру tss */
    memset(&tss, 0, sizeof(tss_entry_t));
    /* Селектор сегмента стека (Stack Segment Selector)
       для уровня привилегий 0 (кольцо 0). */
    tss.ss0 = ss0;
    /* Указатель стека для уровня привилегий 0 */
    tss.esp0 = esp0;
    /* Селектор сегмента кода */
    tss.cs = 0x08;
    /* Селектор сегментов DS, ES, FS, GS */
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x10;
    /* iomap размером в байт и значением запрещающим все */
    tss.iomap = 0xFF;
    /* вычисляем адрес iomap */
    tss.iomap_offset = (uint16_t) ( (uint32_t) &tss.iomap - (uint32_t) &tss );
    /* база tss */
    uint32_t base = (uint32_t) &tss;
    /* limit tss */
    uint32_t limit = sizeof(tss)-1;
    /* Заполняем дескриптор TSS в GDT:
       Создаем указатель на соответствующую запись в GDT для доступа и
       инициализации отдельных полей дескриптора TSS */
    tss_descriptor_t* tss_d = (tss_descriptor_t*) &gdt_entries[num];
    /* Устанавливаем базу и лимит */
    tss_d->base_15_0 = base & 0xFFFF;
    tss_d->base_23_16 = (base >> 16) & 0xFF;
    tss_d->base_31_24 = (base >> 24) & 0xFF;
    tss_d->limit_15_0 = limit & 0xFFFF;
    tss_d->limit_19_16 = (limit >> 16) & 0xF;
    /* Заполняем другие биты */
    tss_d->present = 1; /* Взводим бит присутствия сегмента */
    tss_d->sys = 0;     /* Это не системный сегмент */
    tss_d->DPL = 0;     /* Уровень привилегий сегмента - уровень ядра */
    tss_d->type = 9;    /* Тип сегмента - свободный 32-битный TSS */
    tss_d->AVL = 0;     /* Всегда ноль */
    tss_d->allways_zero = 0; /* Всегда ноль */
    tss_d->gran = 0;    /* Бит гранулярности - ноль */
}

/* Неиспользуется (пока?) */

/* void set_kernel_stack_in_tss(uint32_t stack) { */
/*     tss.esp0 = stack; */
/* } */

/* uint32_t get_tss_esp0(){ */
/*     return tss.esp0; */
/* } */

#define GDT_NUMBER_OF_ELTS 6

void init_gdt(void){
    /* Устанавливается размер GDT в байтах. */
    gdt_ptr.limit = ( sizeof(gdt_entry_t) * GDT_NUMBER_OF_ELTS ) - 1;
    /* Устанавливается базовый адрес GDT */
    gdt_ptr.base = (uint32_t)gdt_entries;

    /* p:0 dpl:0 s:0(sys)
       type:0(Запрещенное значение)
       Нулевой сегмент*/
    gdt_set_gate(0, 0, 0, 0, 0);
    /* p:1 dpl:0 s:1(user)
       type:1010(Сегмент кода для выполнения/чтения)
       Сегмент кода нулевого кольца */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    /* p:1 dpl:0 s:1(user)
       type:0010(Сегмент данных для чтения/записи)
       Сегмент данных нулевого кольца */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    /* p:1 dpl:3 s:1(user)
       type:1010(Сегмент кода для выполнения/чтения)
       Сегмент кода третьего кольца */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    /* p:1 dpl:3 s:1(user)
       type:10(Сегмент данных для чтения/записи)
       Сегмент данных третьего кольца */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* Мы записываем TSS в последний 5-ый гейт  */
    /* Так как дескриптор стека (и данных) ядра имеет
       индекс 2 в GDT, то его селектор, с учетом
       RPL = 0 в GDT будет равен
       stack_selector = 2*8 | RPL = 16
       или 0x10 в hex */
    write_tss(5, 0x10, __init_esp);
    /* Загружаем GDT */
    gdt_flush( (uint32_t) &gdt_ptr);
    /* Загружаем TSS
       Поскольку дескриптор TSS имеет индекс 5 в GDT,
       то его селектор, с учетом RPL = 0, будет равен
       tss_selector = 5*8 | RPL = 40, или 0x28 в hes.
       Именно это значение мы загружаем в TR */
    tss_flush(0x28);
}
