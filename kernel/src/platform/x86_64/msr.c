#include <stdint.h>

uint64_t read_msr(uint32_t msr) {
  uint32_t lo, hi;
  asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
  return ((uint64_t)hi << 32) | lo;
}

void write_msr(uint32_t msr, uint64_t value) {
  uint32_t lo = (uint32_t)(value & 0xFFFFFFFF);
  uint32_t hi = (uint32_t)(value >> 32);
  asm volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}