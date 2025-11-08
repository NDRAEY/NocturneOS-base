#pragma once

#include  "common.h"

struct idt_entry_struct
{
  uint16_t    base_low;
  uint16_t    selector;
  uint8_t     ist;
  uint8_t     flags;
  uint16_t    base_high;
  uint32_t    base_higher;
  uint32_t    reserved;
}__attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

struct    idt_ptr_struct
{
  uint16_t    limit;
  uint64_t    base;
}__attribute__((packed));

typedef   struct  idt_ptr_struct idt_ptr_t;

/* External function for interrupt processing */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);

extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);

extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);

extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

extern void isr80(void);

void init_idt(void);
