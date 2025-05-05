//
// Created by ndraey on 2/10/24.
//

#pragma once

#include "multiboot.h"

void grub_modules_prescan(const multiboot_header_t* hdr);
void grub_modules_init(const multiboot_header_t* hdr);