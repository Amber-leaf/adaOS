#include "header/memmap.h"
#include "../header/core.h"
#include "../header/limine.h"
#include "../util/header/log.h"
#include "../util/header/printf.h"

#include <stddef.h>

const char *LIMINE_MEMMAP_STRINGS[] = {"USABLE",
                                       "RESERVED",
                                       "ACPI_RECLAIMABLE",
                                       "ACPI_NVS",
                                       "BAD_MEMORY",
                                       "BOOTLOADER_RECLAIMABLE",
                                       "KERNEL_AND_MODULES",
                                       "FRAMEBUFFER"};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

struct limine_memmap_response *get_memmap(void) {
  struct limine_memmap_response *memmap = memmap_request.response;
  if (memmap == NULL) {
    k_err("No memmap response!");
    hcf();
  }

  return memmap;
}

void print_mem_segments() {
  struct limine_memmap_response *memmap = get_memmap();

  k_debug("%d entries", memmap->entry_count);
  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    k_debug("seg %d: base: %p, length: %p\ntype: %s", i,
            memmap->entries[i]->base, memmap->entries[i]->length,
            LIMINE_MEMMAP_STRINGS[memmap->entries[i]->type]);
  }
}