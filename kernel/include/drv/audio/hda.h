//
// Created by ndraey on 21.01.24.
//

#pragma once

#include "sys/registers.h"

void hda_init();
void hda_reset();
size_t hda_calculate_entries(size_t word);
void hda_disable_interrupts();
uint32_t hda_send_verb_via_corb_rirb(uint32_t verb);
void hda_interrupt_handler(SAYORI_UNUSED registers_t regs);
void hda_find_afg(size_t codec_response, size_t codec_id);
void hda_initialize_afg();