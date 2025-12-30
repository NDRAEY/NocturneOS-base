/**
 * @file sys/gdt.c
 * @author Пиминов Никита (nikita.piminoff@yandex.ru)
 * @brief (GDT) Глобальная таблица дескрипторов
 * @version 0.4.3
 * @date 2022-10-01
 * @copyright Copyright SayoriOS Team (c) 2022-2026
 */
#include  "arch/x86/gdt.h"
#include  "arch/x86/tss.h"
#include  "io/logging.h"
#include  <arch/x86/ports.h>

extern void gdt_flush(uint32_t);

gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
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
    gdt_entries[num].granularity |= flags << 4;
    /* Access переписываем без изменений */
    gdt_entries[num].access = access;
}

#define GDT_NUMBER_OF_ELTS 6

extern size_t stack_top;

__attribute__((aligned(0x1000)))
uint8_t test_stack[4096];

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
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0b1100);
	/* p:1 dpl:0 s:1(user)
		type:0010(Сегмент данных для чтения/записи)
		Сегмент данных нулевого кольца */
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0b1100);
	/* p:1 dpl:3 s:1(user)
		type:1010(Сегмент кода для выполнения/чтения)
		Сегмент кода третьего кольца */
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0b1100);
	/* p:1 dpl:3 s:1(user)
		type:10(Сегмент данных для чтения/записи)
		Сегмент данных третьего кольца */
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0b1100);

	/* Загружаем GDT */
	gdt_flush( (uint32_t) &gdt_ptr);
	/* Загружаем TSS
	Поскольку дескриптор TSS имеет индекс 5 в GDT,
	то его селектор, с учетом RPL = 0, будет равен
	tss_selector = 5*8 | RPL = 40, или 0x28 в hes.
	Именно это значение мы загружаем в TR */

	// size_t test_sp = (size_t)test_stack + 4096;
	// write_tss(5, 0x10, test_sp);

	write_tss(5, 0x10, (size_t)&stack_top);
	tss_flush(0x28);
}
