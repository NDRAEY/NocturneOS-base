#pragma once

#ifdef NOCTURNE_X86
#include <arch/x86/pit.h>
#endif

#ifdef NOCTURNE_X86_64
#include <arch/x86/pit.h>
#endif

#ifdef NOCTURNE_ARMV7
#include <arch/armv7/timer.h>
#endif