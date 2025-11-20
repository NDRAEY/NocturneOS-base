/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Created by: NotYourFox, 2025 */

#include <arch/x86/cpu_vendors.h>
#include <lib/string.h>
#include <io/ports.h>

extern const struct cpu_vendor *cpu_vendor_start[], *cpu_vendor_end[];
const struct cpu_vendor *const *cpu_vendors = (const struct cpu_vendor* const *)cpu_vendor_start;

#define NR_X86_VENDORS (int)(cpu_vendor_start - cpu_vendor_end)

const struct cpu_vendor cpu_intel = {
    .vendor_name = "Intel",
    .vendor_sig = { CPUID_VENDOR_INTEL },
    
    .vendor = X86_VENDOR_INTEL,
    
    .nr_legacy_models = 4,
    .legacy_models = {
		{ .x86_family = 4, .model_names =
		  {
			  [0] = "486 DX-25/33",
			  [1] = "486 DX-50",
			  [2] = "486 SX",
			  [3] = "486 DX/2",
			  [4] = "486 SL",
			  [5] = "486 SX/2",
			  [7] = "486 DX/2-WB",
			  [8] = "486 DX/4",
			  [9] = "486 DX/4-WB"
		  }
		},
		{ .x86_family = 5, .model_names =
		  {
			  [0] = "Pentium 60/66 A-step",
			  [1] = "Pentium 60/66",
			  [2] = "Pentium 75 - 200",
			  [3] = "OverDrive PODP5V83",
			  [4] = "Pentium MMX",
			  [7] = "Mobile Pentium 75 - 200",
			  [8] = "Mobile Pentium MMX",
			  [9] = "Quark SoC X1000",
		  }
		},
		{ .x86_family = 6, .model_names =
		  {
			  [0] = "Pentium Pro A-step",
			  [1] = "Pentium Pro",
			  [3] = "Pentium II (Klamath)",
			  [4] = "Pentium II (Deschutes)",
			  [5] = "Pentium II (Deschutes)",
			  [6] = "Mobile Pentium II",
			  [7] = "Pentium III (Katmai)",
			  [8] = "Pentium III (Coppermine)",
			  [10] = "Pentium III (Cascades)",
			  [11] = "Pentium III (Tualatin)",
		  }
		},
		{ .x86_family = 15, .model_names =
		  {
			  [0] = "Pentium 4 (Unknown)",
			  [1] = "Pentium 4 (Willamette)",
			  [2] = "Pentium 4 (Northwood)",
			  [4] = "Pentium 4 (Foster)",
			  [5] = "Pentium 4 (Foster)",
		  }
		},
	}
};


const struct cpu_vendor cpu_amd = {
    .vendor_name = "AMD",
    .vendor_sig = { CPUID_VENDOR_AMD, CPUID_VENDOR_AMD_OLD },
    
    .vendor = X86_VENDOR_AMD,

    .nr_legacy_models = 1,
    .legacy_models = {
        { .x86_family = 4, .model_names =
            {
                [3] = "486 DX/2",
                [7] = "486 DX/2-WB",
                [8] = "486 DX/4",
                [9] = "486 DX/4-WB",
                [14] = "Am5x86-WT",
                [15] = "Am5x86-WB",
            }
    	},
    }
};

cpu_register_vendor(cpu_intel);
cpu_register_vendor(cpu_amd);

const struct cpu_vendor* cpu_vendor_by_signature(const char* name) {
	// qemu_log("Vendor count: %x (%x - %x)", NR_X86_VENDORS, cpu_vendor_start, cpu_vendor_end);

	struct cpu_vendor* cpu_vendor;
	for (int vendor = 0; vendor < NR_X86_VENDORS; vendor++) {
		cpu_vendor = cpu_vendors[vendor];
		if (!strcmp(cpu_vendor->vendor_sig[0], name) ||
		    (cpu_vendor->vendor_sig[1] && !strcmp(cpu_vendor->vendor_sig[1], name))) {
			return cpu_vendors[vendor];
		}
	}

	return NULL;
}

const char* cpu_vendor_legacy_model(const struct cpu_vendor* vendor, uint8_t x86_family, uint8_t x86_model) {	
	for (size_t i = 0; i < vendor->nr_legacy_models; i++) {
		if (vendor->legacy_models[i].x86_family == x86_family) {
			return vendor->legacy_models[i].model_names[x86_model];
		}
	}

	return NULL;
}
