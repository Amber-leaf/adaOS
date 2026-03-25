#ifndef MSR_H_
#define MSR_H_

#include <stdint.h>

#define MSR_IA32_EFER 0xC0000080
#define MSR_GSBASE 0xC0000101
#define MSR_IA32_APIC_BASE 0x1B

uint64_t read_msr(uint32_t msr);
void write_msr(uint32_t msr, uint64_t value);

#endif