/*
 * nanovisor - A Minimal Type 2 Hypervisor
 *
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 *
 * This file is licensed under the MIT License.
 */

#include "log.h"
#include <stdbool.h>
#include <stdint.h>

#define FEAT_IDENT_CPUID 0x80000001
#define FEAT_SVM_FEAT_CPUID 0x8000000A
#define FEAT_SVM_OFF 2
#define FEAT_X2AVIC_EXT_OFF 6

#define FEAT_NP 0			    // Nested Paging
#define FEAT_SVML 2			    // SVM Lock
#define FEAT_VMCB_CLEAN 5		    // VMCB Clean Bits
#define FEAT_VMSAVE_VIRT 15		    // VMSAVE and VMLOAD virtualization
#define FEAT_ROGPT 21			    // Read-only Guest Page Tables
#define FEAT_VNMI 25			    // NMI Virtualization
#define FEAT_NESTED_VIRT_VMCB_ADDR_CHECK 28 // Guest VMCB Address Check

#define TEST_SVM_FEAT(val, feat) ((val >> feat) & 0b01)

struct __attribute__((packed)) svm_features {
	uint32_t nasid;
	uint32_t feat_mask;
	bool x2avic_ext;
};

static void print_svm_feat(struct svm_features *feat) {
	pr_debug("mask=%x", feat->feat_mask);
	pr_info_raw("No. of available address space identifiers (NASID) = %d\n",
		    feat->nasid);
	pr_info_raw("x2AVIC_EXT support: %s\n",
		    feat->x2avic_ext ? "yes" : "no");
	uint32_t val = feat->feat_mask;

#define TEST_PRINT_FEAT(val, feat, str)                                        \
	if (TEST_SVM_FEAT(val, FEAT_##feat))                                   \
		pr_info_raw(str ": yes\n");                                    \
	else                                                                   \
		pr_info_raw(str ": no\n");

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

static bool svm_enabled(void) {
	unsigned int status = 0;
	__asm__("mov $0x80000001, %%eax\n"
		"cpuid\n"
		: "=c"(status)::"eax", "ebx", "edx");
	pr_debug("status=%x", status);
	if ((status >> FEAT_SVM_OFF) & 0b01)
		return true;
	else
		return false;
}

int main() {
	bool svm = svm_enabled();
	if (!svm) {
		pr_err("SVM support not enabled");
		return 0;
	}

	pr_info("SVM support enabled");

	struct svm_features svm_feat;
	unsigned int feat_x2avic = 0;
	__asm__("mov $0x8000000a, %%eax\n"
		"cpuid\n"
		: "=b"(svm_feat.nasid), "=c"(feat_x2avic),
		  "=d"(svm_feat.feat_mask)::"eax");
	svm_feat.x2avic_ext = (feat_x2avic >> FEAT_X2AVIC_EXT_OFF) & 0b01;

	print_svm_feat(&svm_feat);
	return 0;
}
