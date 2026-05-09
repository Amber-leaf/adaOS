#ifndef CPUID_H_
#define CPUID_H_
#include <stdbool.h>
#include <stdint.h>

#define NO_HYPERVISOR_TEXT "NOHYPERVISOR"

typedef struct cpuid_regs {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_regs_t;

extern bool check_cpuid();

cpuid_regs_t cpuid(uint32_t leaf, uint32_t subleaf);

char* get_cpu_vendor();
char* get_cpu_name();

char* get_hypervisor_vendor();

#endif