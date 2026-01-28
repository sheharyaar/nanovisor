/*
 * nanovisor - A Minimal Type 2 Hypervisor
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 */
#ifndef _NANOVISOR_H

#include <linux/types.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 15, 11)
#define wrmsrl_on_cpu(cpu, msr, q) wrmsrq_on_cpu(cpu, msr, q)
#endif

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

struct __attribute__((packed)) svm_vmcb_ctrl_area {
	u32 intr_vectors[6]; // 24 bytes
	u8 zeros[36];
	u16 pause_thresh;
	u16 pause_count;
	u64 reserved1;
	u64 reserved2;
	u64 tsc_offset;
	u32 guest_asid;
	u8 tlb_control;
	u8 reserved3[3]; // 24 bits
	u64 virt_intr_ctrl;
	u64 guest_intr;
	u64 exitcode;
	u64 exitinfo1;
	u64 exitinfo2;
	u64 exitintinfo;
	u64 reserved4;
	u64 reserved5; // avic_api_bar
	u64 ghcb_guest_pa;
	u64 eventinj;
	u64 nested_cr3;
	u64 virt_ctrl; // LBR, VMSAVE/VMLOAD enable
	u32 vmcb_clean;
	u32 reserved6;
	u64 nrip;
	u128 instr_stat;
	u64 reserved7; // AVIC APIC_BACKING_PAGE
	u64 reserved8;
	u64 reserved9;	// AVIC_LOGICAL_TABLE
	u64 reserved10; // AVIC_PHYSICAL_TABLE
	u64 reserved11;
	u64 vmsa_pointer;
	u64 vmgexit_rax;
	u64 vmgexit_cpl;
	u64 buslock_thresh_counter;
	u64 reserved12;
	u32 reserved13;
	u32 reserved14; // UPDATE_IRR
	u64 sev_allowed_features;
	u64 sev_guest_features;
	u64 reserved15;
	// // TODO: verify this
	u128 request_irr_lo;
	u128 requested_irr_hi;
	u8 reserved16[624];
	u8 host_use[32];
};

struct __attribute__((packed)) svm_vmcb_segment {
	u16 selector;
	u16 attrib;
	u32 limit;
	u64 base;
};

struct __attribute__((packed)) svm_vmcb_save_area {
	struct svm_vmcb_segment es;
	struct svm_vmcb_segment cs;
	struct svm_vmcb_segment ss;
	struct svm_vmcb_segment ds;
	struct svm_vmcb_segment fs;
	struct svm_vmcb_segment gs;
	struct svm_vmcb_segment gdtr;
	struct svm_vmcb_segment ldtr;
	struct svm_vmcb_segment idtr;
	struct svm_vmcb_segment tr;
	u8 reserved1[43];
	u8 cpl;
	u32 reserved2;
	u64 efer;
	u8 reserved3[16];
	u64 perf_ctl0;
	u64 perf_ctr0;
	u64 perf_ctl1;
	u64 perf_ctr1;
	u64 perf_ctl2;
	u64 perf_ctr2;
	u64 perf_ctl3;
	u64 perf_ctr3;
	u64 perf_ctl4;
	u64 perf_ctr4;
	u64 perf_ctl5;
	u64 perf_ctr5;
	u64 cr4;
	u64 cr3;
	u64 cr0;
	u64 dr7;
	u64 dr6;
	u64 rflags;
	u64 rip;
	u8 reserved4[64];
	u64 instr_retired_ctr;
	u64 perf_ctr_global_sts;
	u64 perf_ctr_global_ctl;
	u64 rsp;
	u64 s_cet;
	u64 ssp;
	u64 isst_addr;
	u64 rax;
	u64 star;
	u64 lstar;
	u64 cstar;
	u64 sfmask;
	u64 kernel_gs_base;
	u64 sysenter_cs;
	u64 sysenter_esp;
	u64 sysenter_eip;
	u64 cr2;
	u8 reserved5[32];
	u64 g_pat;
	u64 dbgctl;
	u64 br_from;
	u64 br_to;
	u64 lastexcpfrom;
	u64 lastexcpto;
	u64 debugextnctl;
	u8 reserved6[64];
	u64 spec_ctrl;
	u8 reserved7[904];
	u8 lbr_stack_from_to[256];
	u64 lbr_select;
	u64 ibs_virt_state[10];
};

struct __attribute__((packed)) svm_vmcb {
	struct svm_vmcb_ctrl_area vmcb_ctrl;
	struct svm_vmcb_save_area vmcb_save;
};

#endif