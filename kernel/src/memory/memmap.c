#include "header/memmap.h"
#include "../header/core.h"
#include "../header/limine.h"
#include "../util/header/log.h"

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

__attribute__((used, section(".limine_requests"))) static volatile struct
    limine_executable_address_request k_addr_request = {
        .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 0};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_hhdm_request
    hhdm_request = {.id = LIMINE_HHDM_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) static volatile struct
    limine_executable_file_request exe_request = {
        .id = LIMINE_EXECUTABLE_FILE_REQUEST, .revision = 0};

struct limine_memmap_response *get_memmap(void) {
  struct limine_memmap_response *memmap = memmap_request.response;
  if (memmap == NULL) {
    panic("No memmap response!");
  }

  return memmap;
}

struct limine_executable_address_response *get_k_addr(void) {
  struct limine_executable_address_response *k_addr = k_addr_request.response;
  if (k_addr == NULL) {
    panic("No kernel address response!");
  }

  return k_addr;
}

struct limine_hhdm_response *get_hhdm(void) {
  struct limine_hhdm_response *hhdm = hhdm_request.response;
  if (hhdm == NULL) {
    panic("No HHDM response!");
  }

  return hhdm;
}

struct limine_executable_file_response *get_exe(void) {
  struct limine_executable_file_response *exe = exe_request.response;
  if (exe == NULL) {
    panic("No executable file response!");
  }

  return exe;
}

bool is_memory_free(int type) {
  return type == LIMINE_MEMMAP_USABLE ||
         type == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
         type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;
}

struct memory_descriptor get_memory_descriptor() {
  struct limine_memmap_response *memmap = get_memmap();

  memory_descriptor_t desc;
  static contiguous_memory_chunk_t free_chunks[MAX_CHUNKS];
  size_t found_chunks = 0;
  desc.length = 0;

  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    uint64_t type = memmap->entries[i]->type;
    uintptr_t base = memmap->entries[i]->base;
    uint64_t length = memmap->entries[i]->length;

    if (is_memory_free(type)) {
      if (found_chunks > 0 && free_chunks[found_chunks - 1].bounds == base) {
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

  // for (int i = 0; i < desc.chunk_count; i++) {
  // k_debug("%d: %p %p", i, desc.chunk_ptr[i].base, desc.chunk_ptr[i].bounds);
  //}

  return desc;
}

void debug_print_mem_map() {
  struct limine_memmap_response *memmap = get_memmap();

  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    uint64_t type = memmap->entries[i]->type;
    uintptr_t base = memmap->entries[i]->base;
    uint64_t length = memmap->entries[i]->length;

    k_debug("seg %d: type: %s. base: %p.\nlength: %p. free: %d", i,
            LIMINE_MEMMAP_STRINGS[type], base, length,
            is_memory_free(type) ? 1 : 0);
  }
}

void print_free_ram() {
  memory_descriptor_t desc = get_memory_descriptor();

  uint64_t length = desc.length;

  uint64_t gib = length / 1073741824;
  uint64_t remainder = length % 1073741824;

  uint64_t decimal = (remainder * 100) / 1073741824;
  k_log("%d.%02d GiB RAM free.", gib, decimal);

  if (gib < 1 && decimal < 50) {
    panic("Insufficient memory. adaOS probably needs \nmore than 0.5GiB of "
          "memory free.");
  }
}