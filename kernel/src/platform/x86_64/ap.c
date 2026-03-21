#include "../../header/limine.h"
#include "../../memory/header/heap.h"
#include "../../util/header/log.h"
#include "stddef.h"

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_mp_request
    mp_request = {.id = LIMINE_MP_REQUEST, .revision = 0};

struct limine_mp_response *get_mp() {
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
    cpu_info.extra_argument = (uint64_t)"Non-Limine generated!";

    struct limine_mp_info cpus[1] = {cpu_info};

    return mp;
  } else {
    return mp;
  }
}

void ap_goto(void *address, uint32_t processor_id) {
  struct limine_mp_response *mp = get_mp();

  for (uint32_t i = 0; i < mp->cpu_count; i++) {
    if (mp->cpus[i]->processor_id == processor_id) {
      mp->cpus[i]->goto_address = address;
      return;
    }
  }

  k_wrn("No such AP %d", processor_id);
}

void setup_multiproc() {
  struct limine_mp_response *mp = get_mp();
  k_debug("our id: %X", mp->bsp_lapic_id);
  k_debug("there are %d CPUs", mp->cpu_count);
  k_debug("x2APIC on? %d", mp->flags);

  for (uint32_t i = 0; i < mp->cpu_count; i++) {
    k_debug("id for cpu %d: %d", i, mp->cpus[i]->processor_id);
    k_debug("lapic id for cpu %d: %d", i, mp->cpus[i]->lapic_id);
  }
}