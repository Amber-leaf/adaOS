#include "header/memmap.h"
#include "../header/core.h"
#include "../header/limine.h"
#include "../util/header/log.h"
#include "../util/header/printf.h"

#include "../util/header/panic.h"

#include <stdbool.h>
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

bool is_memory_free(int type) {
  if (type == 0) { // TODO: || type == 2 || type == 5) {
    return true;
  }
  return false;
}

struct memory_descrtiptor get_memory_descriptor() {
  struct limine_memmap_response *memmap = get_memmap();

  struct memory_descrtiptor desc;
  static struct contiguous_memory_chunk free_chunks[MAX_CHUNKS];
  uint8_t found_chunks = 0;
  desc.length = 0;

  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    uint64_t type = memmap->entries[i]->type;
    uint64_t base = memmap->entries[i]->base;
    uint64_t length = memmap->entries[i]->length;

    if (is_memory_free(type)) {
      if (free_chunks[found_chunks - 1].bounds == base) {
        free_chunks[found_chunks - 1].bounds = base + length;
        desc.length += length;
        continue;
      }

      free_chunks[found_chunks].base = base;
      free_chunks[found_chunks].bounds = base + length;
      desc.length += length;
      found_chunks++;
    }
  }

  desc.chunk_ptr = free_chunks;
  desc.chunk_count = found_chunks;

  for (int i = 0; i < desc.chunk_count; i++) {
    k_debug("%d: %p %p", i, desc.chunk_ptr[i].base, desc.chunk_ptr[i].bounds);
  }

  return desc;
}

void debug_print_mem_map() {
  struct limine_memmap_response *memmap = get_memmap();

  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    uint64_t type = memmap->entries[i]->type;
    uint64_t base = memmap->entries[i]->base;
    uint64_t length = memmap->entries[i]->length;

    k_debug("seg %d: type: %s. base: %p.\nlength: %p. free: %d", i,
            LIMINE_MEMMAP_STRINGS[type], base, length,
            is_memory_free(type) ? 1 : 0);
  }
}

void print_free_ram() {
  struct memory_descrtiptor desc = get_memory_descriptor();

  uint64_t length = desc.length;

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