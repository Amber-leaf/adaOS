#include "header/memmap.h"
#include "../header/core.h"
#include "../header/limine.h"
#include "../util/header/log.h"
#include "../util/header/printf.h"

#include "../util/header/panic.h"

#include <stddef.h>
#include <stdint.h>

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

void fill_free_chunks() {
  struct limine_memmap_response *memmap = get_memmap();

  uint8_t free_segments = 0;

  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    uint64_t type = memmap->entries[i]->type;

    if (type == 0 || type == 2 || type == 5) {
      free_segments += memmap->entries[i]->length;
    }
  }

  struct contiguous_memory_chunk free_chunks[free_segments];

  uint8_t q = 0;

  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    uint64_t type = memmap->entries[i]->type;
    uint64_t base = memmap->entries[i]->base;
    uint64_t length = memmap->entries[i]->length;

    if (type == 0 || type == 2 || type == 5) {
      free_chunks[q].base = base;
      free_chunks[q].bounds = base + length;

      q++;
    }
  }

  for (int i = 0; i < free_segments; i++) {
    printf_("base %p, bounds %p", free_chunks[i].base, free_chunks[i].bounds);
  }
}

uint64_t get_free_ram() {
  struct limine_memmap_response *memmap = get_memmap();

  uint64_t length = 0;

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

  return length;
}

void print_free_ram() {
  uint64_t length = get_free_ram();

  uint32_t length_gib = length / 1073741824;

  uint64_t gib = length / 1073741824;
  uint64_t remainder = length % 1073741824;

  uint64_t decimal = (remainder * 100) / 1073741824;
  k_log("%d.%02d GiB RAM free.", gib, decimal);

  if (length_gib < 0.5) {
    panic("Insufficient memory. adaOS probably needs \nmore than 0.5GiB of "
          "memory free");
  }
}