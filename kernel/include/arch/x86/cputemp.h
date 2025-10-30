//
// Created by ndraey on 02.10.23.
//

#pragma once

#include "common.h"

void cputemp_calibrate();

/**
 * @brief Get CPU temperature on x86 platforms
 * @warning Works only on Intel (R) processors!
 * @return Temperature in Celsius
 */
size_t get_cpu_temperature();
