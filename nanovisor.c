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

static u64 svm_cpu_support_mask = 0;
static u64 svm_cpu_enabled_mask = 0;

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

static bool svm_switch(int cpu, bool enable) {
	// check EFER.SVME if already enabled
	u64 efer_eax = 0;
	rdmsrq_on_cpu(cpu, MSR_EFER, &efer_eax);

	// TODO: test this check for already enabled, why does VirutalBox say
	// already in use when this is loaded in host.
	if (enable) {
		if ((efer_eax >> 12) & 0b01) {
			pr_info("[CPU %d] SVM is already enabled\n", cpu);
			return true;
		}
		efer_eax |= (1 << 12);
	} else {
		if (!((efer_eax >> 12) & 0b01)) {
			pr_info("[CPU %d] SVM is already disabled\n", cpu);
			return true;
		}
		efer_eax &= ~(1 << 12);
	}

	wrmsrq_on_cpu(cpu, MSR_EFER, efer_eax);

	rdmsrq_on_cpu(cpu, MSR_EFER, &efer_eax);
	if (enable) {
		if ((efer_eax >> 12) & 0b01) {
			pr_info("[CPU %d] SVM enabled successfully\n", cpu);
			return true;
		}
	} else {
		if (!((efer_eax >> 12) & 0b01)) {
			pr_info("[CPU %d] SVM disabled successfully\n", cpu);
			return true;
		}
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
static bool svm_support_avail(int cpu) {
	// check for SVM support in CPU
	struct cpuinfo_x86 *c = &cpu_data(cpu);
	if (!cpu_has(c, X86_FEATURE_SVM)) {
		pr_err("[CPU %d] SVM not available on the system\n", cpu);
		return false;
	}

	// check if SVM is dsiabled on firmware level
	u64 vm_cr = 0;
	rdmsrq_on_cpu(cpu, MSR_VM_CR, &vm_cr);
	if (!((vm_cr >> 4) & 0b01)) {
		return true; // it's not disabled (+ its supported by CPU)
	}

	if (cpu_has(c, X86_FEATURE_SVML)) {
		pr_info("[CPU %d] SVM can be unlocked using SVM Key, consult "
			"TPM or platform firmware for the key.\n",
			cpu);
	} else {
		pr_info("[CPU %d] SVM must be enabled from frimware / BIOS\n",
			cpu);
	}

	return false;
}

static int __init nano_init(void) {
	pr_info("Nanovisor Loaded\n");
	int cpu;

	for_each_online_cpu(cpu) {
		if (svm_support_avail(cpu)) {
			svm_cpu_support_mask |= (1 << cpu);
			pr_info("[CPU %d] SVM is supported\n", cpu);

			if (svm_switch(cpu, true)) {
				svm_cpu_enabled_mask |= (1 << cpu);
			} else {
				pr_err("[CPU %d] failed to enable SVM\n", cpu);
			}
		}
	}

	return 0;
}

static void __exit nano_exit(void) {
	int cpu;
	for_each_online_cpu(cpu) {
		if ((svm_cpu_enabled_mask >> cpu) & 0b01)
			svm_switch(cpu, false);
	}
	pr_info("Nanovisor Unloaded\n");
}

module_init(nano_init);
module_exit(nano_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>");
MODULE_DESCRIPTION("Minimal Hypervisor");
