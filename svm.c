/*
 * nanovisor - A Minimal Type 2 Hypervisor
 *
 * Copyright (c) 2026 Mohammad Shehar Yaar Tausif <sheharyaar48@gmail.com>
 *
 * This file is licensed under the MIT License.
 */

#include "log.h"
#include <stdbool.h>

#define DEBUG 1 // force debug printing
#define SVM_BIT_OFF 2

static bool svm_enabled(void)
{
    unsigned int status = 0;
    __asm__("mov $0x80000001, %%eax\n"
            "cpuid\n"
            "mov %%ecx, %0\n" : "=r"(status));
    pr_debug("status=%x", status);
    if ((status >> SVM_BIT_OFF) & 0b01)
        return true;
    else
        return false;
}

int main()
{
    bool svm = svm_enabled();
    if (!svm)
    {
        printf("SVM support not enabled\n");
        return 0;
    }

    printf("SVM support enabled\n");

    unsigned int a, b, c, d;
    __asm__(
        "mov $0x80000001, %%eax\n"
        "cpuid\n"
        "mov %%eax, %0\n"
        "mov %%ebx, %1\n"
        "mov %%ecx, %2\n"
        "mov %%edx, %3\n"
        : "=r"(a), "=r"(b), "=r"(c), "=r"(d));

    printf("a=%x, b=%x, c=%x, d=%x", a, b, c, d);
    return 0;
}
