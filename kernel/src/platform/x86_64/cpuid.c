#include "header/cpuid.h"

#include <stdbool.h>
#include <stdint.h>

#include "../../header/core.h"
#include "header/cpuid.h"

cpuid_regs_t cpuid(uint32_t leaf, uint32_t subleaf) {
  cpuid_regs_t regs;
  __asm__ volatile("cpuid"
                   : "=a"(regs.eax), "=b"(regs.ebx), "=c"(regs.ecx),
                     "=d"(regs.edx)
                   : "a"(leaf), "c"(subleaf));
  return regs;
}

int cpu_feature_bit(uint32_t leaf, uint32_t subleaf, uint8_t reg,
                    uint32_t bit) {
  if (bit > 31) {
    return 0;
  }

  struct cpuid_regs r = cpuid(leaf, subleaf);

  uint32_t value;

  switch (reg) {
  case 'a':
    value = r.eax;
    break;
  case 'b':
    value = r.ebx;
    break;
  case 'c':
    value = r.ecx;
    break;
  case 'd':
    value = r.edx;
    break;
  default:
    return 0;
  }

  return (value >> bit) & 1;
}

char *get_cpu_vendor() {
  static char vendor[13];
  struct cpuid_regs r = cpuid(0, 0);

  memcpy(&vendor[0], &r.ebx, 4);
  memcpy(&vendor[4], &r.edx, 4);
  memcpy(&vendor[8], &r.ecx, 4);
  vendor[12] = '\0';

  return vendor;
}

char *get_hypervisor_vendor() {
  if (cpu_feature_bit(1, 0, 'c', 0x1F)) {
    static char hv_vendor[13];
    struct cpuid_regs r = cpuid(0x40000000, 0);

    memcpy(&hv_vendor[0], &r.ebx, 4);
    memcpy(&hv_vendor[4], &r.ecx, 4);
    memcpy(&hv_vendor[8], &r.edx, 4);
    hv_vendor[12] = '\0';

    return hv_vendor;
  } else {
    return NO_HYPERVISOR_TEXT;
  }
}

char *get_cpu_name() {
  struct cpuid_regs r = cpuid(0x80000000, 0);
  if (r.eax < 0x80000004) {
    return "NO CPU NAME";
  }

  static char name[49];

  struct cpuid_regs regs;
  char *p = name;

  for (uint32_t i = 0x80000002; i <= 0x80000004; i++) {
    regs = cpuid(i, 0);
    memcpy(p, &regs.eax, 4);
    p += 4;
    memcpy(p, &regs.ebx, 4);
    p += 4;
    memcpy(p, &regs.ecx, 4);
    p += 4;
    memcpy(p, &regs.edx, 4);
    p += 4;
  }

  name[48] = '\0';
  return name;
}
