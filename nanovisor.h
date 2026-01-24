/*
 * nanovisor - A Minimal Type 2 Hypervisor
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 */
#ifndef _NANOVISOR_H

#include <linux/types.h>

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