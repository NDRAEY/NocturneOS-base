#pragma once

#include "common.h"

struct    tss_entry {
    uint32_t  prev_tss;

    uint32_t  esp0;          /* Указатель текущего стека ядра */
    uint32_t  ss0;           /* Селектор сегмента текущего стека ядра */
    
    uint32_t  esp1;
    uint32_t  ss1;
    
    uint32_t  esp2;
    uint32_t  ss2;
    
    uint32_t  cr3;
    uint32_t  eip;
    uint32_t  eflags;
    uint32_t  eax;
    uint32_t  ecx;
    uint32_t  edx;
    uint32_t  ebx;
    uint32_t  esp;
    uint32_t  ebp;
    uint32_t  esi;
    uint32_t  edi;
    uint32_t  es;
    uint32_t  cs;
    uint32_t  ss;
    uint32_t  ds;
    uint32_t  fs;
    uint32_t  gs;
    uint32_t  ldtr;
    uint16_t  trap;
    uint16_t  iomap_base; //  I/O Map Base Address Field
} __attribute__((packed));

typedef struct tss_entry tss_entry_t;

struct tss_descriptor
{
    uint16_t  limit_15_0;       /* Биты 15-0 лимита */
    uint16_t  base_15_0;        /* Биты 15-0 базы */
    uint8_t   base_23_16;       /* Биты 23-16 базы */
    uint8_t   type:4;           /* Тип сегмента */
    uint8_t   sys:1;            /* Системный сегмент */
    uint8_t   DPL:2;            /* Уровень привилегий сегмента */
    uint8_t   present:1;        /* Бит присутствия */
    uint8_t   limit_19_16:4;    /* Биты 19-16 лимита */
    uint8_t   AVL:1;            /* Зарезервирован */
    uint8_t   always_zero:2;   /* Всегда нулевые */
    uint8_t   gran:1;           /* Бит гранулярности */
    uint8_t   base_31_24;       /* Биты 31-24 базы */
}__attribute__((packed));

typedef struct tss_descriptor tss_descriptor_t;

extern tss_entry_t tss;

void write_tss(int32_t num, uint32_t ss0, uint32_t esp0);
void tss_flush(uint32_t tr_selector);