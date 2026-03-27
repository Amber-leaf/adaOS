#include "header/cpu.h"
#include "../../header/core.h"
#include "../../header/limine.h"
#include "../../memory/header/heap.h"
#include "../../memory/header/memmap.h"
#include "../../memory/physical/header/pmm.h"
#include "../../memory/virtual/header/vmm.h"
#include "../../platform/x86_64/header/apic.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"
#include "../../util/header/printf.h"
#include "../../util/header/state.h"

#include "header/msr.h"

#include "stddef.h"
#include <stdbool.h>
#include <stdint.h>

cpu_t **smp_cpus;

size_t cpus_awake = 1;
cpu_id_t id = 1;

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_mp_request
    mp_request = {.id = LIMINE_MP_REQUEST, .revision = 0};

cpu_t inline *get_cpu(void) { return (cpu_t *)read_msr(MSR_GSBASE); }

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

void awake(struct limine_mp_info *info) {
  set_cpu((cpu_t *)info->extra_argument); // We stashed the cpu context in the
                                          // extra argument the Limine gives us.
  get_cpu()->lapic_base = read_msr(MSR_IA32_APIC_BASE);
  get_cpu()->id = __atomic_fetch_add(&id, 1, __ATOMIC_SEQ_CST);

  switch_kernel_pagemap();

  bootstrap_apic();

  get_cpu()->lapic_id = read_apic_register(APIC_ID);

  k_debug("Setup CPU %d", get_cpu()->id);

  __atomic_fetch_add(&cpus_awake, 1, __ATOMIC_SEQ_CST);

  hcf();
}

void setup_cpus() {
  struct limine_mp_response *mp = get_mp_info();

  k_debug("our id: %X", mp->bsp_lapic_id);
  k_debug("there are %d CPUs", mp->cpu_count);
  k_debug("x2APIC on? %d", mp->flags);

  // use physical pages so the other cpus have it on the hhdm
  size_t cpu_size = ALIGN_UP(sizeof(cpu_t) * mp->cpu_count, PAGE_SIZE);

  if (cpu_size > PAGE_SIZE) {
    panic("TODO: let pp_alloc allocate more than one page contiguously");
  }

  cpu_t *cpus = HIGHER_HALF + pp_alloc();
  cpu_t *bsp_c = kmalloc(sizeof(cpu_t));

  struct limine_mp_info *bsp_info;

  smp_cpus = kmalloc(mp->cpu_count * sizeof(cpu_t *));

  for (int i = 0; i < mp->cpu_count; ++i) {
    bool bsp = mp->cpus[i]->lapic_id == mp->bsp_lapic_id;

    if (bsp) {
      bsp_c->lapic_id = mp->bsp_lapic_id;
      bsp_c->id = 0;

      bsp_info = mp->cpus[i];

      smp_cpus[i] = bsp_c;
      mp->cpus[i]->extra_argument = (uintptr_t)&bsp_c;

      set_cpu(bsp_c);
    } else {
      smp_cpus[i] = &cpus[i];
      mp->cpus[i]->extra_argument = (uintptr_t)&cpus[i];
      __atomic_store_n(&mp->cpus[i]->goto_address, awake, __ATOMIC_SEQ_CST);
    }
  }

  set_state(SMP_INITIALIZED, 0xff);

  while (__atomic_load_n(&cpus_awake, __ATOMIC_SEQ_CST) != mp->cpu_count) {
    asm("pause");
  }

  k_debug("Started other CPUs");
}
