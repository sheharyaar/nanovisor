/*
 * nanovisor - A Minimal Type 2 Hypervisor
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 */
#ifndef _NANOVISOR_H

#include <linux/types.h>

/* CPUID related defines for checking SVM support*/

#define CPUID_FEAT 0x80000001
#define CPUID_FEAT_SVM 0x8000000A

#define FEAT_NP 0			    // Nested Paging
#define FEAT_SVML 2			    // SVM Lock
#define FEAT_VMCB_CLEAN 5		    // VMCB Clean Bits
#define FEAT_VMSAVE_VIRT 15		    // VMSAVE and VMLOAD virtualization
#define FEAT_ROGPT 21			    // Read-only Guest Page Tables
#define FEAT_VNMI 25			    // NMI Virtualization
#define FEAT_NESTED_VIRT_VMCB_ADDR_CHECK 28 // Guest VMCB Address Check

/* MSRs involved in SVM (from Appendix A, Setion A.7) */

#define MSR_EFER 0xc0000080
/* Guest View of Timestamp Counter, Section 15.30.5 */
#define MSR_TSC_RATIO 0xc0010104
/* Controls global aspect of SVM, Section 15.30.1 */
#define MSR_VM_CR 0xc0010114
/* IGNNE Signal, Section 15.30.2 */
#define MSR_IGNNE 0xc0010115
/* Control over SMM Signals, Section 15.30.3 */
#define MSR_SMM_CTL 0xc0010116
/* VM Host State Save Physical Address, Section 15.30.4 */
#define MSR_VM_HSAVE_PA 0xc0010117
#define MSR_SVM_KEY 0xc0010118
#define MSR_DOORBELL 0xc001011B	    /* Doorbell register */
#define MSR_VMPAGE_FLUSH 0xc001011B /* SEV */
#define MSR_GHCB 0xc0010130	    /* GHCB */
#define MSR_SEV_STATUS 0xc0010131
#define MSR_RMP_BASE 0xc0010132
#define MSR_RMP_END 0xc0010133
#define MSR_GUEST_TSC_FREQ 0xc0010134	 /* Secure TSC */
#define MSR_VIRTUAL_TOM 0xc0010135	 /* Virtual Top of Memory */
#define MSR_SEGMENTED_RMP_CFG 0xc0010136 /* Segmented RMP */
#define MSR_IDLE_WAKEUP_ICR 0xc0010137	 /* Side Channel Protection */
#define MSR_SECURE_AVIC_CTRL 0xc0010138	 /* Secure AVIC */

struct __attribute__((packed)) svm_features {
	uint32_t nasid;
	uint32_t feat_mask;
	bool x2avic_ext;
};

#endif