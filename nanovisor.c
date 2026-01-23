/*
 * nanovisor - A Minimal Type 2 Hypervisor
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 */
#include "nanovisor.h"

static void print_svm_feat(struct svm_features *feat) {
	pr_debug("mask=%x", feat->feat_mask);
	pr_info("Address Space Identifiers (NASID): %d\n", feat->nasid);
	pr_info("x2AVIC_EXT support: %s\n",
		    feat->x2avic_ext ? "yes" : "no");
	uint32_t val = feat->feat_mask;

#define TEST_PRINT_FEAT(val, feat, str)                                        \
	if (TEST_SVM_FEAT(val, FEAT_##feat))                                   \
		pr_info(str ": yes\n");                                    \
	else                                                                   \
		pr_info(str ": no\n");

	TEST_PRINT_FEAT(val, NP, "Nested Paging");
	TEST_PRINT_FEAT(val, SVML, "SVM Lock");
	TEST_PRINT_FEAT(val, VMCB_CLEAN, "VMCB Clean bits");
	TEST_PRINT_FEAT(val, VMSAVE_VIRT, "VMSAVE and VMLOAD");
	TEST_PRINT_FEAT(val, ROGPT, "Read-Only Guest Page Tables");
	TEST_PRINT_FEAT(val, VNMI, "NMI Virtualisation");
	TEST_PRINT_FEAT(val, NESTED_VIRT_VMCB_ADDR_CHECK,
			"Guest VMCB Address Check");
#undef TEST_PRINT_FEAT
}

// check support for SVM and see if it can be enabled
static bool svm_enabled(void) {
	// check EFER.SVME if already enabled
	uint32_t efer = 0;
	__asm__(
		"rdmsr\n"
		: "=a"(efer)
		: "c"(MSR_EFER)
		: "edx"
	);

	bool svme_status = (efer >> 12) & 0b01;
	if (svme_status) {
		pr_info("SVM is already enabled");
		return true;
	}


	// The CPU does not feature SVM
	uint32_t status = 0;
	__asm__("cpuid\n"
			: "=c"(status)
			: "a"(CPUID_FEAT)
			: "ebx", "edx");
	
			pr_debug("status=%x", status);
	if (!((status >> FEAT_SVM_OFF) & 0b01)) {
		pr_err("SVM not available on the system");
		return false;
	}
	
	// The CPU features, check if it's disabled
	uint32_t vm_cr = 0;
	bool svmdis = 1;
	__asm__(
		"rdmsr\n"
		: "=a"(vm_cr)
		: "c"(MSR_VM_CR)
		: "edx"
	);

	pr_debug("vm_cr: %#x", vm_cr);
	svmdis = (vm_cr >> 4) & 0b01;

	struct svm_features svm_feat;
	unsigned int feat_x2avic = 0;
	__asm__(
		"cpuid\n"
		: "=b"(svm_feat.nasid), "=c"(feat_x2avic),
		  "=d"(svm_feat.feat_mask)
		: "a"(CPUID_FEAT_SVM)
		:);
	svm_feat.x2avic_ext = (feat_x2avic >> FEAT_X2AVIC_EXT_OFF) & 0b01;

	// SVM is disabled, check if it can be unlocked
	if (svmdis) {
		if ((svm_feat.feat_mask >> FEAT_SVML) & 0b01) {
			pr_info("SVM can be unlocked using SVM Key, consult TPM or platform firmware for the key.");
			return false;
		} else {
			pr_info("SVM must be enabled from frimware / BIOS");
			return false;
		}
	}

	// Try to enable SVM
	// The CPU features, check if it's disabled
	// efer |= (1 << 12);
	// __asm__(
	// 	"wrmsr\n"
	// 	: 
	// 	: "c"(MSR_VM_CR), "a"(efer)
	// 	: "edx"
	// );

	print_svm_feat(&svm_feat);
	return true;
}

static int __init nano_init(void)
{
	pr_info("Nanovisor Loaded\n");
	if (!svm_enabled()) {
		// Operation not permitted ? Or other erro code
		return 0;
	}

	pr_info("SVM support enabled");

	return 0;
}

static void __exit nano_exit(void)
{
	pr_info("Nanovisor Unloaded\n");
}

module_init(nano_init);
module_exit(nano_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>");
MODULE_DESCRIPTION("Minimal Hypervisor");

