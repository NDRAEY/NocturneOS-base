/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Created by: NotYourFox, 2025 */

#include <lib/bitops.h>
#include <arch/bitops.h>
#include <arch/x86/asm.h>

__attribute__((const)) int bit_ls(unsigned long x) {
	int res;

	if (__builtin_constant_p(x)) return x ? BITS_PER_LONG - __builtin_clz(x) : 0;

	asm(__asm_op("bsr", BYTES_PER_LONG) " %1,%0\n\t"
	    "jnz 1f\n\t"
	    "movl $-1,%0\n"
	    "1:"
	    : "=r"(res)
	    : "rm"(x));

	return res + 1;
}
