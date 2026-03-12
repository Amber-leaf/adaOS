#include "header/acpi.h"
#include "../../header/core.h"
#include "../../header/limine.h"
#include "../../memory/header/heap.h"
#include "../../memory/header/memmap.h"
#include "../../memory/virtual/header/vmm.h"

#include "../../util/header/log.h"
#include "header/apic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST, .revision = 0};

extern pagemap_t *kernel_pagemap;

struct limine_rsdp_response *get_rsdp() {
  struct limine_rsdp_response *rsdp = rsdp_request.response;

  if (rsdp != NULL && rsdp->address) {
    return rsdp;
  }
  k_err("Failed getting RSDP! ACPI likely not supported.");
  return NULL;
}

void bootstrap_acpi() {
  struct limine_rsdp_response *rsdp = get_rsdp();

  uintptr_t phys = rsdp->address;
  uintptr_t virt = phys + HIGHER_HALF;
  if (!map_page(kernel_pagemap, virt, phys, PTE_PCD | PTE_NX)) {
    k_err("Could not map RSDP!");
    return;
  }

  k_debug("phys: %p, virt: %p", phys, virt);
  k_debug("page offset: %p", phys & 0xFFF);

  struct rsdp_header rsdp_h = *(struct rsdp_header *)virt;

  // memcpy(&rsdp_h, (void *)virt, sizeof(rsdp_h));

  k_debug("sig: %.8s", rsdp_h.signature);
};