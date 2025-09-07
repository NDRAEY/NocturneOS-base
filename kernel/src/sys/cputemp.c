#include "arch/x86/cpu_vendors.h"
#include "arch/x86/cpufeature.h"
#include "sys/msr.h"
#include <sys/cpuid.h>

// X86 CPU Temperature support (partially based on memtest86+ code)

/* 
 * TODO: make a structure with function pointers for similar operations for each CPU vendor.
 * For example, one could easily do:
 *      cpu_vendors[X86_VENDOR_INTEL]->ops.cputemp_calibrate();
 * or
 *      cpu_vendors[X86_VENDOR_AMD]->ops.cputemp_calibrate();
 * and that would be good design.
 */

size_t cputemp_calibrate_value = 0;

/**
 * @brief Get a special value
 * @warning Works only on Intel (R) processors!
 */
void cputemp_calibrate() {
	uint32_t a = 0;
	uint32_t b = 0;
	
	if(boot_cpu_has(X86_FEATURE_DTHERM) && boot_cpu_info.manufacturer_id == X86_VENDOR_INTEL) {
		rdmsr(INTEL_TEMPERATURE_TARGET, a, b);
		cputemp_calibrate_value = (a >> 16) & 0x7F;

		// From memtest86+ code

		if (cputemp_calibrate_value < 50 || cputemp_calibrate_value > 125) {
			cputemp_calibrate_value = 100;
		}
	}
}

/**
 * @brief Get CPU temperature on x86 platforms
 * @warning Works only on Intel (R) processors!
 * @return Temperature in Celsius
 */
size_t get_cpu_temperature() {
	if(boot_cpu_has(X86_FEATURE_DTHERM) && boot_cpu_info.manufacturer_id == X86_VENDOR_INTEL) {
		uint32_t a = 0;
		uint32_t b = 0;

		rdmsr(INTEL_THERMAL_STATUS, a, b);

		uint32_t absolute = (a >> 16) & 0x7F;

		return cputemp_calibrate_value - absolute;
	}

	return 0;
}
