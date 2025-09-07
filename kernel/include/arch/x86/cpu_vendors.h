/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Created by: NotYourFox, 2025 */

#pragma once

#include <common.h>

#define CPUID_VENDOR_AMD "AuthenticAMD"
#define CPUID_VENDOR_AMD_OLD "AMDisbetter!" // Early engineering samples of AMD K5 processor
#define CPUID_VENDOR_INTEL "GenuineIntel"
#define CPUID_VENDOR_VIA "VIA VIA VIA "
#define CPUID_VENDOR_TRANSMETA "GenuineTMx86"
#define CPUID_VENDOR_TRANSMETA_OLD "TransmetaCPU"
#define CPUID_VENDOR_CYRIX "CyrixInstead"
#define CPUID_VENDOR_CENTAUR "CentaurHauls"
#define CPUID_VENDOR_NEXGEN "NexGenDriven"
#define CPUID_VENDOR_UMC "UMC UMC UMC "
#define CPUID_VENDOR_SIS "SiS SiS SiS "
#define CPUID_VENDOR_NSC "Geode by NSC"
#define CPUID_VENDOR_RISE "RiseRiseRise"
#define CPUID_VENDOR_VORTEX "Vortex86 SoC"
#define CPUID_VENDOR_AO486 "MiSTer AO486"
#define CPUID_VENDOR_AO486_OLD "GenuineAO486"
#define CPUID_VENDOR_ZHAOXIN "  Shanghai  "
#define CPUID_VENDOR_HYGON "HygonGenuine"
#define CPUID_VENDOR_ELBRUS "E2K MACHINE "

// Vendor strings from hypervisors.
#define CPUID_VENDOR_QEMU "TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM "KVMKVMKVM\0\0\0"
#define CPUID_VENDOR_KVM_ALT "Linux KVM Hv"
#define CPUID_VENDOR_VMWARE "VMwareVMware"
#define CPUID_VENDOR_VIRTUALBOX "VBoxVBoxVBox"
#define CPUID_VENDOR_XEN "XenVMMXenVMM"
#define CPUID_VENDOR_HYPERV "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS " prl hyperv "
#define CPUID_VENDOR_PARALLELS_ALT " lrpepyh vr "
#define CPUID_VENDOR_BHYVE "bhyve bhyve "
#define CPUID_VENDOR_QNX " QNXQVMBSQG "

enum {
    X86_VENDOR_AMD,
    X86_VENDOR_AMD_OLD,
    X86_VENDOR_INTEL,
    X86_VENDOR_VIA,
    X86_VENDOR_TRANSMETA,
    X86_VENDOR_TRANSMETA_OLD,
    X86_VENDOR_CYRIX,
    X86_VENDOR_CENTAUR,
    X86_VENDOR_NEXGEN,
    X86_VENDOR_UMC,
    X86_VENDOR_SIS,
    X86_VENDOR_NSC,
    X86_VENDOR_RISE,
    X86_VENDOR_VORTEX,
    X86_VENDOR_AO486,
    X86_VENDOR_AO486_OLD,
    X86_VENDOR_ZHAOXIN,
    X86_VENDOR_HYGON,
    X86_VENDOR_ELBRUS,
    
    X86_VENDOR_QEMU,
    X86_VENDOR_KVM,
    X86_VENDOR_KVM_ALT,
    X86_VENDOR_VMWARE,
    X86_VENDOR_VIRTUALBOX,
    X86_VENDOR_XEN,
    X86_VENDOR_HYPERV,
    X86_VENDOR_PARALLELS,
    X86_VENDOR_PARALLELS_ALT,
    X86_VENDOR_BHYVE,
    X86_VENDOR_QNX
};

struct cpu_vendor {
	const char* vendor_name;
	const char* vendor_sig[2];

	uint8_t vendor;
	
	size_t nr_legacy_models;
	
	struct {
		uint8_t x86_family;
		const char* model_names[16];
	} legacy_models[];
};

#define cpu_register_vendor(sym)                                                                   \
	static const struct cpu_vendor* const __attribute__((section(".cpu_vendor_data")))             \
	__attribute__((used)) __cpu_vendor_##sym = &sym;

extern const struct cpu_vendor *const *cpu_vendors;

const struct cpu_vendor* cpu_vendor_by_signature(const char* name);
const char* cpu_vendor_legacy_model(const struct cpu_vendor* vendor, uint8_t x86_family, uint8_t x86_model);
