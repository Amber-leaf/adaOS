#include "header/memmap.h"
#include "../header/core.h"
#include "../header/limine.h"
#include "../util/header/log.h"
#include "../util/header/panic.h"
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
    panic("No memmap response!");
  }

  return memmap;
}

void print_free_ram() {
  struct limine_memmap_response *memmap = get_memmap();

  uint64_t length;

  k_debug("%d entries", memmap->entry_count);
  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    k_debug("seg %d: base: %p, length: %p\ntype: %s", i,
            memmap->entries[i]->base, memmap->entries[i]->length,
            LIMINE_MEMMAP_STRINGS[memmap->entries[i]->type]);

    uint64_t type = memmap->entries[i]->type;
    if (type == 0 || type == 2 || type == 5) {
      length += memmap->entries[i]->length;
    }
  }
  uint32_t length_gib = length / 1073741824;

  uint64_t gib = length / 1073741824ULL;
  uint64_t remainder = length % 1073741824ULL;

  uint64_t decimal = (remainder * 100) / 1073741824ULL;
  k_log("%d.%02d GiB RAM free.", gib, decimal);

  if (length_gib < 0.5) {
    panic("Insufficient memory. adaOS needs more memory free than 0.5GiB");
  }
}