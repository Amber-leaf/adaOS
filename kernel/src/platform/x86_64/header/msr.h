#ifndef MSR_H_
#define MSR_H_

#include <stdint.h>

#define IA32_EFER 0xC0000080

uint64_t read_msr(uint32_t msr);

#endif