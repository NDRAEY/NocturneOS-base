#include <common.h>
#include <arch/x86_64/idt64.h>
#include <arch/x86/ports.h>
#include <lib/string.h>

extern void idt_flush(uint64_t);

idt_entry_t  idt_entries[256];
idt_ptr_t    idt_ptr;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags){
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].base_higher = (base >> 32) & 0xFFFFFFFF;
    idt_entries[num].selector = selector;
    idt_entries[num].flags = flags;
}

void init_idt(void) {
    /* Инициализация структуры указателя размером и адресом IDT */
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (uint32_t)idt_entries;
    /* Очистка памяти */
    memset(idt_entries, 0, sizeof(idt_entry_t) * 256);

    /* Создание записей в таблице прерываний */
    idt_set_gate(0, (size_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (size_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (size_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (size_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (size_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (size_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (size_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (size_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (size_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (size_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (size_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (size_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (size_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (size_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (size_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (size_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (size_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (size_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (size_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (size_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (size_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (size_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (size_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (size_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (size_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (size_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (size_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (size_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (size_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (size_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (size_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (size_t)isr31, 0x08, 0x8E);
    /* IRQs */
    idt_set_gate(32, (size_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (size_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (size_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (size_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (size_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (size_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (size_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (size_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (size_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (size_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (size_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (size_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (size_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (size_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (size_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (size_t)irq15, 0x08, 0x8E);
    /* System calls */
    idt_set_gate(0x80, (size_t)isr80, 0x08, 0xEE);
    /* Загружаем */
    idt_flush((uint64_t) &idt_ptr);


    // PIC initialization

    /* Инициализация первого PIC и установка режима ICW1
    (Initialization Command Word 1). */
    outb(0x20, 0x11);
    /* Инициализация второго PIC и установка режима ICW1. */
    outb(0xA0, 0x11);
    /* Установка базового вектора прерывания для первого PIC (ICW2). */
    outb(0x21, 0x20);
    /* Установка базового вектора прерывания для второго PIC (ICW2). */
    outb(0xA1, 0x28);
    /* Конфигурация мастер-системы (ICW3 для первого PIC). */
    outb(0x21, 0x04);
    /* Конфигурация слейв-системы (ICW3 для второго PIC). */
    outb(0xA1, 0x02);
    /* Установка режима работы (ICW4 для первого PIC). */
    outb(0x21, 0x01);
    /* Установка режима работы (ICW4 для второго PIC). */
    outb(0xA1, 0x01);
    /* Окончательная настройка маски прерываний для первого PIC. */
    outb(0x21, 0x0);
    /* Окончательная настройка маски прерываний для второго PIC. */
    outb(0xA1, 0x0);
}