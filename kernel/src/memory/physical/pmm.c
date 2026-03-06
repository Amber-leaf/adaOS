#include "header/pmm.h"
#include "../header/memmap.h"

#include "../../header/core.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"

#include <stdbool.h>
#include <stdint.h>

#define USED false
#define FREE true

#define MAX_SEGMENTS 32

#define VMM_HIGHER_HALF get_hhdm()->offset

uint8_t page_bitmap[MAX_PAGES / 8]; // if set then used

struct page_mapping page_mappings[MAX_SEGMENTS];

uint32_t last_used_mapping = 0;

uint32_t mapped_in_segment = 0;

uint64_t last_page_used = 0;

uint64_t free_pages = MAX_PAGES;

uint64_t used_pages = 0;

struct memory_descriptor desc;

uint64_t get_used_memory() {
    uint64_t used = used_pages;
    return used * PAGE_SIZE;
}

uint64_t get_free_memory() {
    uint64_t free = free_pages;
    return free * PAGE_SIZE;
}

// check whether a physical page is free, see pp_free() for deallocating.
bool page_free(uint64_t page_index) {
  uint64_t byte = page_index / 8;
  uint8_t bit = page_index % 8;

  if ((page_bitmap[byte] >> bit) & 1) {
    return false;
  }

  return true;
}

void set_page(uint64_t page_index, bool free) {
  uint64_t byte = page_index / 8;
  uint8_t bit = page_index % 8;

  if (free) {
    page_bitmap[byte] &= ~(1 << bit);
    free_pages++;
    used_pages--;
  } else {
    k_debug("physical page %d allocated", page_index);
    page_bitmap[byte] |= (1 << bit);
    last_page_used = page_index;
    free_pages--;
    used_pages++;
  }
}

// get the "best" free page's index
uint64_t get_free_page() {
  if (last_page_used >= get_k_addr()->physical_base) {
    last_page_used = 0;
  }

  if (last_page_used >= MAX_PAGES) {
    last_page_used = MAX_PAGES - 1;
  }

  if (page_free(last_page_used++)) {
    return last_page_used;
  }

  // TODO: do this better!
  for (uint64_t i = 0; i <= MAX_PAGES; i++) {
    if (page_free(i)) {
      last_page_used = i;
      return i;
    }
  }
  // TODO: Handle this properly.
  panic("Out of memory");
  return -1;
}

void generate_page_mapping() {
  uint64_t pages_used = 0;
  for (uint32_t i = 0; i < desc.chunk_count; i++) {
    struct contiguous_memory_chunk chunk = desc.chunk_ptr[i];

    page_mappings[i].offset = chunk.base;
    page_mappings[i].page_start_index = pages_used;
    page_mappings[i].page_end_index =
        pages_used + ((chunk.bounds - chunk.base) / PAGE_SIZE);

    pages_used += ((chunk.bounds - chunk.base) / PAGE_SIZE) + 1;
  }
}

uint64_t ptr_to_page(void *ptr) {
  struct page_mapping mapping;
  bool wrapped = false;

  if (ptr == NULL) {
    k_err("NULL pointer passed to ptr_to_page!");
    return -1;
  }

  if ((uint64_t)ptr % PAGE_SIZE != 0) {
    k_err("Non-page alined pointer passed to ptr_to_page!");
    return -1;
  }

retry:
  mapping = page_mappings[last_used_mapping];

  uint64_t segment_size =
      (mapping.page_end_index - mapping.page_start_index) * PAGE_SIZE;

  if (ptr > (void *)mapping.offset &&
      ptr < (void *)(mapping.offset + segment_size)) {

    return mapping.page_start_index +
           (uint64_t)(ptr - mapping.offset) / PAGE_SIZE;

  } else if (last_used_mapping <= desc.chunk_count) {
    last_used_mapping++;
    goto retry;
  }

  if (!wrapped) {
    last_used_mapping = 0;
    wrapped = true;
    goto retry;
  }

  return -1;
}

void *page_to_ptr(uint64_t page_index) {
  struct page_mapping mapping;
  bool wrapped = false;

retry:
  mapping = page_mappings[last_used_mapping];

  if (page_index > mapping.page_start_index &&
      page_index < mapping.page_end_index) {
    return (void *)(mapping.offset +
                    (page_index - mapping.page_start_index) * PAGE_SIZE);

  } else if (last_used_mapping <= desc.chunk_count) {
    last_used_mapping++;
    goto retry;
  }

  if (!wrapped) {
    last_used_mapping = 0;
    wrapped = true;
    goto retry;
  }

  k_err("Failed to find mapping for physical page %d!", page_index);
  k_debug("last used mapping: %d", last_used_mapping);

  return NULL;
}

void *_pp_alloc(uint64_t page_index) {
  if (page_index >= get_k_addr()->physical_base) {
    k_err("tried to allocate a page (%d) that was in the kernel executable's "
          "space!",
          page_index);
    return NULL;
  }

  if (page_index > MAX_PAGES) {
    k_err("tried to allocate physical page %d at %p that was outside the "
          "bounds of paged memory!",
          page_index, page_to_ptr(page_index));
    return NULL;
  }

  if (page_free(page_index)) {
    set_page(page_index, USED);

    void *addr = page_to_ptr(page_index);

    void *vaddr = (void *)((uintptr_t)addr + VMM_HIGHER_HALF);

    memset(vaddr, 0, PAGE_SIZE);

    return addr;
  }
  k_wrn("tried to allocate a page (%d) that was not free!", page_index);
  return NULL;
}

void *pp_alloc() {
  uint64_t index = get_free_page();
  return _pp_alloc(index);
}

void _pp_free(uint64_t page_index) {
  if (page_index > MAX_PAGES) {
    k_err("tried to free physical page %d at %p that was outside the bounds of "
          "paged memory!",
          page_index, page_to_ptr(page_index));
    return;
  }

  if (page_free(page_index)) {
    k_wrn("tried to free physical page %d that was allready free!", page_index);
    return;
  }

  k_debug("physical page %d freed", page_index);

  set_page(page_index, FREE);
}

void pp_free(void *ptr) {
  uint64_t page_index = ptr_to_page(ptr);

  _pp_free(page_index);
}

void setup_pmm() {
  desc = get_memory_descriptor();
  generate_page_mapping();

  for (uint32_t i = 0; i < desc.chunk_count; i++) {
    k_debug("pg mp %d: addr: %p st pg: %d end pg: %d", i,
            page_mappings[i].offset, page_mappings[i].page_start_index,
            page_mappings[i].page_end_index);
  }

  k_debug("k physical addr: %p, virt addr: %p", get_k_addr()->physical_base,
          get_k_addr()->virtual_base);

  k_debug("hhdm offset %p", get_hhdm()->offset);
}
