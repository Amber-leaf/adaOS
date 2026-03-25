#include "header/cpu.h"
#include "../../header/core.h"
#include "../../header/limine.h"
#include "../../memory/header/heap.h"
#include "../../memory/header/memmap.h"
#include "../../memory/physical/header/pmm.h"
#include "../../util/header/log.h"
#include "../../util/header/printf.h"
#include "header/msr.h"

#include "stddef.h"
#include <stdint.h>

cpu_t **smp_cpus;

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_mp_request
    mp_request = {.id = LIMINE_MP_REQUEST, .revision = 0};

cpu_t inline *get_cpu(void) {
  cpu_t *cpu;
  // asm volatile("mov %%gs:0, %%rax" : "=a"(cpu) : : "memory"); Astral does it
  // like this but idk why, read_msr works while this faults so...
  cpu = (cpu_t *)read_msr(MSR_GSBASE);

  return cpu;
}

void inline set_cpu(cpu_t *ptr) { write_msr(MSR_GSBASE, (uint64_t)ptr); }

struct limine_mp_response *get_mp_info() {
  struct limine_mp_response *mp = mp_request.response;
  if (mp == NULL) {
    k_err("Failed to get MP info from Limine! Failing safe and asuming only "
          "bootstrap proc.");
    mp->cpu_count = 1;
    mp->bsp_lapic_id = 0;
    mp->flags = 0;
    mp->revision = 2;

    struct limine_mp_info cpu_info;
    cpu_info.lapic_id = 0;
    cpu_info.processor_id = 0;
    cpu_info.goto_address = NULL;

    struct limine_mp_info cpus[1] = {cpu_info};

    return mp;
  } else {
    return mp;
  }
}

uint64_t next_id = 1;

void awake(struct limine_mp_info *info) {
  set_cpu((cpu_t *)info->extra_argument); // We stashed the cpu context in the
                                          // extra argument the Limine gives us.
  get_cpu()->lapic_base = read_msr(MSR_IA32_APIC_BASE);
  get_cpu()->local_id = info->lapic_id;

  k_debug("lapic base: %p", get_cpu()->lapic_base);
  k_debug("lapic id: %d", get_cpu()->local_id);

  k_debug("hello world from cpu %d!", get_cpu()->local_id);

  hcf();
}

void setup_cpus() {
  struct limine_mp_response *mp = get_mp_info();

  k_debug("our id: %X", mp->bsp_lapic_id);
  k_debug("there are %d CPUs", mp->cpu_count);
  k_debug("x2APIC on? %d", mp->flags);

  // use physical pages so the other cpus have it on the hhdm
  size_t cpu_size = ALIGN_UP(sizeof(cpu_t) * mp->cpu_count, PAGE_SIZE);
  k_debug("bad: %d", cpu_size > PAGE_SIZE);

  cpu_t *cpus = HIGHER_HALF + pp_alloc();

  smp_cpus = kmalloc(mp->cpu_count * sizeof(cpu_t *));

  for (int i = 0; i < mp->cpu_count; ++i) {
    // skip the bootstrap processor
    if (mp->cpus[i]->lapic_id == mp->bsp_lapic_id) {
      continue;
    }

    smp_cpus[i] = &cpus[i];

    mp->cpus[i]->extra_argument = (uint64_t)&cpus[i];

    __atomic_store_n(&mp->cpus[i]->goto_address, awake, __ATOMIC_SEQ_CST);
  }
  size_t arch_smp_cpusawake = 1;

  while (__atomic_load_n(&arch_smp_cpusawake, __ATOMIC_SEQ_CST) !=
         mp->cpu_count)
    asm("pause");

  k_debug("awoke other processors\n");
}
