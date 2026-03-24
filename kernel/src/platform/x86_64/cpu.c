#include "header/cpu.h"

cpu_t *this_cpu(void) {
  cpu_t *cpu;
  asm volatile("mov %%gs:0, %%rax" : "=a"(cpu) : : "memory");
  return cpu;
}