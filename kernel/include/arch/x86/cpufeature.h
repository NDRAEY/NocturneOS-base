#pragma once

enum cpuid_leafs {
	CPUID_1_ECX,
	CPUID_1_EDX,

	CPUID_6_EAX,
	
	CPUID_7_EBX,

	CPUID_80000007_EDX,

	NR_CPUID_LEAFS
};

/* Bit indexes for x86_capability mask */

/* CPUID leaf 0x00000001 (ecx), word 0 */
#define X86_FEATURE_PNI (0 * 32 + 0)                 /* Streaming SIMD Extensions 3 (SSE3) */
#define X86_FEATURE_PCLMULQDQ (0 * 32 + 1)           /* PCLMULQDQ instruction support */
#define X86_FEATURE_DTES64 (0 * 32 + 2)              /* 64-bit DS save area */
#define X86_FEATURE_MONITOR (0 * 32 + 3)             /* MONITOR/MWAIT support */
#define X86_FEATURE_DS_CPL (0 * 32 + 4)              /* CPL Qualified Debug Store */
#define X86_FEATURE_VMX (0 * 32 + 5)                 /* Virtual Machine Extensions */
#define X86_FEATURE_SMX (0 * 32 + 6)                 /* Safer Mode Extensions */
#define X86_FEATURE_EST (0 * 32 + 7)                 /* Enhanced Intel SpeedStep */
#define X86_FEATURE_TM2 (0 * 32 + 8)                 /* Thermal Monitor 2 */
#define X86_FEATURE_SSSE3 (0 * 32 + 9)               /* Supplemental SSE3 */
#define X86_FEATURE_CID (0 * 32 + 10)                /* L1 Context ID */
#define X86_FEATURE_SDBG (0 * 32 + 11)               /* Silicon Debug */
#define X86_FEATURE_FMA (0 * 32 + 12)                /* FMA extensions using YMM state */
#define X86_FEATURE_CX16 (0 * 32 + 13)               /* CMPXCHG16B instruction support */
#define X86_FEATURE_XTPR (0 * 32 + 14)               /* xTPR Update Control */
#define X86_FEATURE_PDCM (0 * 32 + 15)               /* Perfmon and Debug Capability */
#define X86_FEATURE_PCID (0 * 32 + 17)               /* Process-context identifiers */
#define X86_FEATURE_DCA (0 * 32 + 18)                /* Direct Cache Access */
#define X86_FEATURE_SSE4_1 (0 * 32 + 19)             /* SSE4.1 */
#define X86_FEATURE_SSE4_2 (0 * 32 + 20)             /* SSE4.2 */
#define X86_FEATURE_X2APIC (0 * 32 + 21)             /* X2APIC support */
#define X86_FEATURE_MOVBE (0 * 32 + 22)              /* MOVBE instruction support */
#define X86_FEATURE_POPCNT (0 * 32 + 23)             /* POPCNT instruction support */
#define X86_FEATURE_TSC_DEADLINE_TIMER (0 * 32 + 24) /* APIC timer one-shot operation */
#define X86_FEATURE_AES (0 * 32 + 25)                /* AES instructions */
#define X86_FEATURE_XSAVE (0 * 32 + 26)              /* XSAVE (and related instructions) support */
#define X86_FEATURE_OSXSAVE (0 * 32 + 27) /* XSAVE (and related instructions) are enabled by OS */
#define X86_FEATURE_AVX (0 * 32 + 28)     /* AVX instructions support */
#define X86_FEATURE_F16C (0 * 32 + 29)    /* Half-precision floating-point conversion support */
#define X86_FEATURE_RDRAND (0 * 32 + 30)  /* RDRAND instruction support */
#define X86_FEATURE_HYPERVISOR                                                                     \
	(0 * 32 + 31) /* System is running as guest; (para-)virtualized system */

/* CPUID leaf 0x00000001 (edx), word 1 */
#define X86_FEATURE_FPU (1 * 32 + 0)      /* Floating-Point Unit on-chip (x87) */
#define X86_FEATURE_VME (1 * 32 + 1)      /* Virtual-8086 Mode Extensions */
#define X86_FEATURE_DE (1 * 32 + 2)       /* Debugging Extensions */
#define X86_FEATURE_PSE (1 * 32 + 3)      /* Page Size Extension */
#define X86_FEATURE_TSC (1 * 32 + 4)      /* Time Stamp Counter */
#define X86_FEATURE_MSR (1 * 32 + 5)      /* Model-Specific Registers (RDMSR and WRMSR support) */
#define X86_FEATURE_PAE (1 * 32 + 6)      /* Physical Address Extensions */
#define X86_FEATURE_MCE (1 * 32 + 7)      /* Machine Check Exception */
#define X86_FEATURE_CX8 (1 * 32 + 8)      /* CMPXCHG8B instruction */
#define X86_FEATURE_APIC (1 * 32 + 9)     /* APIC on-chip */
#define X86_FEATURE_SEP (1 * 32 + 11)     /* and associated MSRs */
#define X86_FEATURE_MTRR (1 * 32 + 12)    /* Memory Type Range Registers */
#define X86_FEATURE_PGE (1 * 32 + 13)     /* Page Global Extensions */
#define X86_FEATURE_MCA (1 * 32 + 14)     /* Machine Check Architecture */
#define X86_FEATURE_CMOV (1 * 32 + 15)    /* Conditional Move Instruction */
#define X86_FEATURE_PAT (1 * 32 + 16)     /* Page Attribute Table */
#define X86_FEATURE_PSE36 (1 * 32 + 17)   /* Page Size Extension (36-bit) */
#define X86_FEATURE_PSN (1 * 32 + 18)     /* Processor Serial Number */
#define X86_FEATURE_CLFLUSH (1 * 32 + 19) /* CLFLUSH instruction */
#define X86_FEATURE_DS (1 * 32 + 21)      /* Debug Store */
#define X86_FEATURE_ACPI (1 * 32 + 22)    /* Thermal monitor and clock control */
#define X86_FEATURE_MMX (1 * 32 + 23)     /* MMX instructions */
#define X86_FEATURE_FXSR (1 * 32 + 24)    /* FXSAVE and FXRSTOR instructions */
#define X86_FEATURE_SSE (1 * 32 + 25)     /* SSE instructions */
#define X86_FEATURE_SSE2 (1 * 32 + 26)    /* SSE2 instructions */
#define X86_FEATURE_SS (1 * 32 + 27)      /* Self Snoop */
#define X86_FEATURE_HT (1 * 32 + 28)      /* Hyper-threading */
#define X86_FEATURE_TM (1 * 32 + 29)      /* Thermal Monitor */
#define X86_FEATURE_IA64 (1 * 32 + 30)    /* now reserved */
#define X86_FEATURE_PBE (1 * 32 + 31)     /* Pending Break Enable */

/* CPUID leaf 0x00000006 (eax), word 2 */
#define X86_FEATURE_DTHERM                 		(6 * 32 + 0)	/* Digital temperature sensor */
#define X86_FEATURE_TURBO_BOOST            		(6 * 32 + 1)	/* Intel Turbo Boost */
#define X86_FEATURE_ARAT                   		(6 * 32 + 2)	/* Always-Running APIC Timer (not affected by p-state) */
#define X86_FEATURE_PLN                    		(6 * 32 + 4)	/* Power Limit Notification (PLN) event */
#define X86_FEATURE_ECMD                   		(6 * 32 + 5)	/* Clock modulation duty cycle extension */
#define X86_FEATURE_PTS                    		(6 * 32 + 6)	/* Package thermal management */
#define X86_FEATURE_HWP                    		(6 * 32 + 7)	/* HWP (Hardware P-states) base registers are supported */
#define X86_FEATURE_HWP_NOTIFY             		(6 * 32 + 8)	/* HWP notification (IA32_HWP_INTERRUPT MSR) */
#define X86_FEATURE_HWP_ACT_WINDOW         		(6 * 32 + 9)	/* HWP activity window (IA32_HWP_REQUEST[bits 41:32]) supported */
#define X86_FEATURE_HWP_EPP                		(6 * 32 + 10)	/* HWP Energy Performance Preference */
#define X86_FEATURE_HWP_PKG_REQ            		(6 * 32 + 11)	/* HWP Package Level Request */
#define X86_FEATURE_HDC_BASE_REGS          		(6 * 32 + 13)	/* HDC base registers are supported */
#define X86_FEATURE_TURBO_BOOST_3_0        		(6 * 32 + 14)	/* Intel Turbo Boost Max 3.0 */
#define X86_FEATURE_HWP_CAPABILITIES       		(6 * 32 + 15)	/* HWP Highest Performance change */
#define X86_FEATURE_HWP_PECI_OVERRIDE      		(6 * 32 + 16)	/* HWP PECI override */
#define X86_FEATURE_HWP_FLEXIBLE           		(6 * 32 + 17)	/* Flexible HWP */
#define X86_FEATURE_HWP_FAST               		(6 * 32 + 18)	/* IA32_HWP_REQUEST MSR fast access mode */
#define X86_FEATURE_HFI                    		(6 * 32 + 19)	/* HW_FEEDBACK MSRs supported */
#define X86_FEATURE_HWP_IGNORE_IDLE        		(6 * 32 + 20)	/* Ignoring idle logical CPU HWP req is supported */
#define X86_FEATURE_THREAD_DIRECTOR        		(6 * 32 + 23)	/* Intel thread director support */
#define X86_FEATURE_THERM_INTERRUPT_BIT25  		(6 * 32 + 24)	/* IA32_THERM_INTERRUPT MSR bit 25 is supported */

/* CPUID leaf 0x00000007:0 (ebx), word 3 */
#define X86_FEATURE_FSGSBASE (2 * 32 + 0)   /* FSBASE/GSBASE read/write support */
#define X86_FEATURE_TSC_ADJUST (2 * 32 + 1) /* IA32_TSC_ADJUST MSR supported */
#define X86_FEATURE_SGX (2 * 32 + 2)        /* Intel SGX (Software Guard Extensions) */
#define X86_FEATURE_BMI1 (2 * 32 + 3)       /* Bit manipulation extensions group 1 */
#define X86_FEATURE_HLE (2 * 32 + 4)        /* Hardware Lock Elision */
#define X86_FEATURE_AVX2 (2 * 32 + 5)       /* AVX2 instruction set */
#define X86_FEATURE_FDP_EXCPTN_ONLY                                                                \
	(2 * 32 + 6)                      /* FPU Data Pointer updated only on x87 exceptions */
#define X86_FEATURE_SMEP (2 * 32 + 7) /* Supervisor Mode Execution Protection */
#define X86_FEATURE_BMI2 (2 * 32 + 8) /* Bit manipulation extensions group 2 */
#define X86_FEATURE_ERMS (2 * 32 + 9) /* Enhanced REP MOVSB/STOSB */
#define X86_FEATURE_INVPCID                                                                        \
	(2 * 32 + 10)                     /* INVPCID instruction (Invalidate Processor Context ID) */
#define X86_FEATURE_RTM (2 * 32 + 11) /* Intel restricted transactional memory */
#define X86_FEATURE_CQM (2 * 32 + 12) /* Intel RDT-CMT / AMD Platform-QoS cache monitoring */
#define X86_FEATURE_ZERO_FCS_FDS (2 * 32 + 13) /* Deprecated FPU CS/DS (stored as zero) */
#define X86_FEATURE_MPX (2 * 32 + 14)          /* Intel memory protection extensions */
#define X86_FEATURE_RDT_A (2 * 32 + 15)        /* Intel RDT / AMD Platform-QoS Enforcement */
#define X86_FEATURE_AVX512F (2 * 32 + 16)      /* AVX-512 foundation instructions */
#define X86_FEATURE_AVX512DQ (2 * 32 + 17)     /* AVX-512 double/quadword instructions */
#define X86_FEATURE_RDSEED (2 * 32 + 18)       /* RDSEED instruction */
#define X86_FEATURE_ADX (2 * 32 + 19)          /* ADCX/ADOX instructions */
#define X86_FEATURE_SMAP (2 * 32 + 20)         /* Supervisor mode access prevention */
#define X86_FEATURE_AVX512IFMA (2 * 32 + 21)   /* AVX-512 integer fused multiply add */
#define X86_FEATURE_CLFLUSHOPT (2 * 32 + 23)   /* CLFLUSHOPT instruction */
#define X86_FEATURE_CLWB (2 * 32 + 24)         /* CLWB instruction */
#define X86_FEATURE_INTEL_PT (2 * 32 + 25)     /* Intel processor trace */
#define X86_FEATURE_AVX512PF (2 * 32 + 26)     /* AVX-512 prefetch instructions */
#define X86_FEATURE_AVX512ER (2 * 32 + 27)     /* AVX-512 exponent/reciprocal instructions */
#define X86_FEATURE_AVX512CD (2 * 32 + 28)     /* AVX-512 conflict detection instructions */
#define X86_FEATURE_SHA_NI (2 * 32 + 29)       /* SHA/SHA256 instructions */
#define X86_FEATURE_AVX512BW (2 * 32 + 30)     /* AVX-512 byte/word instructions */
#define X86_FEATURE_AVX512VL (2 * 32 + 31)     /* AVX-512 VL (128/256 vector length) extensions */

/* CPUID leaf 0x80000007 (edx), word 4 */
#define X86_FEATURE_DIGITAL_TEMP (3 * 32 + 0)       /* Digital temperature sensor */
#define X86_FEATURE_POWERNOW_FREQ_ID (3 * 32 + 1)   /* PowerNOW! frequency scaling */
#define X86_FEATURE_POWERNOW_VOLT_ID (3 * 32 + 2)   /* PowerNOW! voltage scaling */
#define X86_FEATURE_THERMAL_TRIP (3 * 32 + 3)       /* THERMTRIP (Thermal Trip) */
#define X86_FEATURE_HW_THERMAL_CONTROL (3 * 32 + 4) /* Hardware thermal control */
#define X86_FEATURE_SW_THERMAL_CONTROL (3 * 32 + 5) /* Software thermal control */
#define X86_FEATURE_100MHZ_STEPS (3 * 32 + 6)       /* 100 MHz multiplier control */
#define X86_FEATURE_HW_PSTATE (3 * 32 + 7)          /* Hardware P-state control */
#define X86_FEATURE_INVARIANT_TSC                                                                  \
	(3 * 32 + 8)                     /* TSC ticks at constant rate across all P and C states */
#define X86_FEATURE_CPB (3 * 32 + 9) /* Core performance boost */
#define X86_FEATURE_EFF_FREQ_RO (3 * 32 + 10)       /* Read-only effective frequency interface */
#define X86_FEATURE_PROC_FEEDBACK (3 * 32 + 11)     /* Processor feedback interface (deprecated) */
#define X86_FEATURE_ACC_POWER (3 * 32 + 12)         /* Processor power reporting interface */
#define X86_FEATURE_CONNECTED_STANDBY (3 * 32 + 13) /* CPU Connected Standby support */
#define X86_FEATURE_RAPL (3 * 32 + 14)              /* Runtime Average Power Limit interface */

/* Custom-defined features, word 4 */
#define X86_FEATURE_CONSTANT_TSC (4 * 32 + 0)   /* TSC ticks at a constant rate */
#define X86_FEATURE_NONSTOP_TSC (4 * 32 + 1)    /* TSC does not stop in C-states */
#define X86_FEATURE_TSC_KNOWN_FREQ (4 * 32 + 2) /* TSC has known frequency */
