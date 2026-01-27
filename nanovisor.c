/*
 * nanovisor - A Minimal Type 2 Hypervisor
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 */
#include "nanovisor.h"
#include <asm/cpufeature.h>
#include <asm/io.h>
#include <asm/msr.h>
#include <asm/set_memory.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>

static u64 svm_cpu_support_mask = 0;
static u64 svm_cpu_enabled_mask = 0;
unsigned long long svm_vmcb_page = NULL;
unsigned long long svm_host_save_area_page = 0;

static int svm_vmcb_setup(void) {
	if (sizeof(struct svm_vmcb_ctrl_area) != 1024) {
		pr_err("VMCB control areas has wrong size\n");
		return -1;
	}

	if (sizeof(struct svm_vmcb_save_area) != 1992) {
		pr_err("VMCB save areas has wrong size\n");
		return -1;
	}
	return 0;
}

static int svm_run(void) {
	// 1. Put processor in protected mode, efer.svme to 1 (already done ??)
	unsigned long long vmcb_pa = virt_to_phys(svm_vmcb_page);
	unsigned long long save_area_pa = virt_to_phys(svm_host_save_area_page);

	if (!vmcb_pa || !save_area_pa) {
		return -1;
	}

	// set 'MSR_VM_HASVE_PA'
	wrmsrq(MSR_VM_HSAVE_PA, save_area_pa);

	// set the GIF to 0
	asm("clgi" ::: "memory");

	// set to protected mode (CR0.PE)
	unsigned long long cr0 = 0;
	asm("movq %%cr0, %%rax" : "=a"(cr0)::);
	cr0 |= 0b01;
	asm("movq %%rax, %%cr0" ::"a"(cr0) :);

	if (svm_vmcb_setup() != 0)
		goto intr_err;

	// set VMCB initial states
	asm("vmrun\n" : : "a"(vmcb_pa) :);

	// reset to host values
	return 0;

intr_err:
	asm("stgi" ::: "memory");
	return -1;
}

// VMCB address should be physical address, and 4-KB aligned and mapped as
// writeback (WB)
static int svm_run_prep(void) {
	svm_vmcb_page = (struct svm_vmcb *)get_zeroed_page(GFP_KERNEL);
	if (!svm_vmcb_page)
		return -ENOMEM;

	pr_debug("page allocated for vmcb: KVA(%px), PA(%px)\n",
		 (void *)svm_vmcb_page, (void *)virt_to_phys(svm_vmcb_page));

	// mark the page as wb
	if (set_memory_wb(svm_vmcb_page, 1) != 0) {
		pr_err("error in setting VMCB page as writeback\n");
		goto clean_vmcb;
	}
	pr_debug("page marked as wb\n");

	svm_save_area_page = (struct svm_vmcb *)get_zeroed_page(GFP_KERNEL);
	if (!svm_save_area_page)
		goto clean_vmcb;

	pr_debug("page allocated for save_area: KVA(%px), PA(%px)\n",
		 (void *)svm_save_area_page,
		 (void *)virt_to_phys(svm_save_area_page));

	// mark the page as wb
	if (set_memory_wb(svm_save_area_page, 1) != 0) {
		pr_err("error in setting save_area page as writeback\n");
		goto clean_save_area;
	}
	pr_debug("page marked as wb\n");

	return 0;

clean_save_area:
	free_page(svm_save_area_page);
	svm_save_area_page = 0;

clean_vmcb:
	free_page(svm_vmcb_page);
	svm_vmcb_page = 0;
	return -1;
}

static bool svm_switch(int cpu, bool enable) {
	// check EFER.SVME if already enabled
	u32 efer_eax = 0;
	u32 efer_eax_h = 0;
	rdmsr_on_cpu(cpu, MSR_EFER, &efer_eax, &efer_eax_h);

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

	wrmsr_on_cpu(cpu, MSR_EFER, efer_eax, 0);

	rdmsr_on_cpu(cpu, MSR_EFER, &efer_eax, &efer_eax_h);
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
	u32 vm_cr = 0;
	u32 vm_cr_h = 0;
	rdmsr_on_cpu(cpu, MSR_VM_CR, &vm_cr, &vm_cr_h);
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

	// enable SVME on each CPU
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

	pr_debug("preparing vmcb for vmrun\n");
	if (svm_run_prep() != 0) {
		pr_err("svm_run_prep failed\n");
		return -ENOMEM;
	}

	svm_run();

	return 0;
}

static void __exit nano_exit(void) {
	int cpu;
	for_each_online_cpu(cpu) {
		if ((svm_cpu_enabled_mask >> cpu) & 0b01)
			svm_switch(cpu, false);
	}

	if (svm_vmcb_ptr) {
		free_page((unsigned long)svm_vmcb_ptr);
		svm_vmcb_ptr = NULL;
	}
	pr_info("Nanovisor Unloaded\n");
}

module_init(nano_init);
module_exit(nano_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>");
MODULE_DESCRIPTION("Minimal Hypervisor");
