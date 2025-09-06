/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Created by: NotYourFox, 2025 */

#pragma once

#include <common.h>
#include <lib/math.h>
#include <stdbool.h>
#include <arch/bitops.h>

#ifdef SAYORI64
#define BYTES_PER_LONG 8
#else
#define BYTES_PER_LONG 4
#endif

#define BITS_PER_BYTE   8
#define BITS_PER_LONG   (BYTES_PER_LONG * BITS_PER_BYTE)

#define BIT_MASK(bit)        (1UL << ((bit) % BITS_PER_LONG))
#define BIT_WORD(bit)        ((bit) / BITS_PER_LONG)

#define bit_fs(n) __builtin_ffs(n)

static inline void set_bit(unsigned long* addr, unsigned long bit) {
    unsigned long* p = ((unsigned long*)addr + BIT_WORD(bit));
    *p |= BIT_MASK(bit);
}

static inline void clear_bit(unsigned long* addr, unsigned long bit) {
    unsigned long* p = ((unsigned long*)addr + BIT_WORD(bit));
    *p &= ~BIT_MASK(bit);
}

static inline bool test_bit(unsigned long* addr, unsigned long bit) {
    unsigned long* p = ((unsigned long*)addr + BIT_WORD(bit));
    return !!(*p & BIT_MASK(bit));
}

#define BITMAP(name, bits) unsigned long name[bits / BITS_PER_LONG]

#define BITMAP_FIND_NEXT(map, MUNGE, size, start) ({                    \
    unsigned long res = size;                                           \
    if (start >= size) goto out;                                        \
                                                                        \
    unsigned long chunk;                                                \
    unsigned long byte_pos = start / BITS_PER_LONG;                     \
    for (                                                               \
        chunk = MUNGE(map[byte_pos] & ((~0UL) << start));               \
        !chunk;                                                         \
        chunk = MUNGE(map[++byte_pos])                                  \
    ) {                                                                 \
        /* Exit loop before we unwantedly read chunk past the end       \
         * If this is the last chunk and we loop, this means the        \
         * chunk is 0, so we didn't find the bit */                     \
        if ((byte_pos + 1) * BITS_PER_LONG >= size) goto out;           \
    }                                                                   \
                                                                        \
    /* bit_fs returns bit pos + 1. We are sure that we have found       \
     * a non-zero chunk, so no need to check bit_fs. */                 \
    res = MIN(byte_pos * BITS_PER_LONG + bit_fs(chunk) - 1, size);      \
                                                                        \
out:                                                                    \
    res;                                                                \
})

/*
 * Searches for next set bit. If not found, returns size.
 */
static inline unsigned long bitmap_find_set(unsigned long* bitmap, unsigned long size, unsigned long start) {
    return BITMAP_FIND_NEXT(bitmap, /* nop */, size, start);
}

/*
 * Searches for next clear bit. If not found, returns size.
 */
static inline unsigned long bitmap_find_clear(unsigned long* bitmap, unsigned long size, unsigned long start) {
    return BITMAP_FIND_NEXT(bitmap, ~, size, start);
}

#define for_each_set_bit(pos, bitmap, size) \
    for ((pos) = bitmap_find_set((bitmap), (size), 0); (pos) < (size); (pos) = bitmap_find_set((bitmap), (size), (pos) + 1))
