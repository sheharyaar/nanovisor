/*
 * nanovisor - A Minimal Type 2 Hypervisor
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 */
#include "nanovisor.h"
#include <asm/cpufeature.h>
#include <asm/msr.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>

static void svm_features_print(void) {
#define TEST_PRINT_FEAT(feat, str)                                             \
	if (boot_cpu_has(feat))                                                \
		pr_info(str ": yes\n");                                        \
	else                                                                   \
		pr_info(str ": no\n");

	TEST_PRINT_FEAT(X86_FEATURE_X2AVIC, "X2AVIC Support");
	TEST_PRINT_FEAT(X86_FEATURE_NPT, "Nested Paging");
	TEST_PRINT_FEAT(X86_FEATURE_SVML, "SVM Lock");
	TEST_PRINT_FEAT(X86_FEATURE_VMCBCLEAN, "VMCB Clean bits");
	TEST_PRINT_FEAT(X86_FEATURE_V_VMSAVE_VMLOAD, "VMSAVE and VMLOAD");
	TEST_PRINT_FEAT(X86_FEATURE_VNMI, "NMI Virtualisation");
	TEST_PRINT_FEAT(X86_FEATURE_SVME_ADDR_CHK, "Guest VMCB Address Check");

#undef TEST_PRINT_FEAT
}

static bool svm_enable(void) {
	// check EFER.SVME if already enabled
	u64 efer_eax = 0;
	rdmsrl(MSR_EFER, efer_eax);
	pr_debug("efer_eax=%#llx\n", efer_eax);

	if ((efer_eax >> 12) & 0b01) {
		pr_info("SVM is already enabled\n");
		return true;
	}

	// Try to enable SVM
	efer_eax |= (1 << 12);
	wrmsrl(MSR_EFER, efer_eax);

	rdmsrl(MSR_EFER, efer_eax);
	pr_debug("efer_eax new=%#llx\n", efer_eax);
	if ((efer_eax >> 12) & 0b01) {
		pr_info("SVM enabled successfully\n");
		return true;
	}

	// Should not reach here
	BUG_ON(1);
	return false;
}

/*
Check for SVM Support:
	- First check if the CPU supports SVM, if yes, (0x8000_0001ECX[SVM])
	- Ccheck if it is allowed (VM_CR.SVMDIS)
		- If allowed, then can be enabled
		- If not allowed, check if it is locked (0x8000_000A_EDX[SVML])
			- If == 0, SVM is disabled at BIOS level
			- Else ==11, can be unlocked using key
*/
static bool svm_support_avail(void) {
	// check for SVM support in CPU
	if (!boot_cpu_has(X86_FEATURE_SVM)) {
		pr_err("SVM not available on the system\n");
		return false;
	}

	// check if SVM is dsiabled on firmware level
	uint32_t vm_cr = 0;
	rdmsrl(MSR_VM_CR, vm_cr);
	if (!((vm_cr >> 4) & 0b01)) {
		return true; // it's not disabled (+ its supported by CPU)
	}

	if (boot_cpu_has(X86_FEATURE_SVML)) {
		pr_info("SVM can be unlocked using SVM Key, consult "
			"TPM or platform firmware for the key.\n");
	} else {
		pr_info("SVM must be enabled from frimware / BIOS\n");
	}

	return false;
}

static int __init nano_init(void) {
	pr_info("Nanovisor Loaded\n");
	if (!svm_support_avail()) {
		return -1;
	}

	pr_info("SVM is supported, with the following feature status:\n");
	svm_features_print();

	pr_debug("Enabling SVM\n");

	if (!svm_enable()) {
		return -1;
	}

	return 0;
}

static void __exit nano_exit(void) { pr_info("Nanovisor Unloaded\n"); }

module_init(nano_init);
module_exit(nano_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>");
MODULE_DESCRIPTION("Minimal Hypervisor");
