#include "header/bitmap_alloc.h"
#include "../header/memmap.h"

#include "../../util/header/log.h"
#include "../../util/header/panic.h"
#include "../../util/header/printf.h"

#include <stdbool.h>
#include <stdint.h>

#define USED false
#define FREE true

#define PAGE_SIZE 4096
#define MAX_PAGES 1024 * 1024

#define SUPPORTED_MEM MAX_PAGES *PAGE_SIZE

#define MAX_SEGMENTS 32

uint8_t page_bitmap[MAX_PAGES / 8]; // if set then used

struct page_mapping page_mappings[MAX_SEGMENTS];

uint32_t last_used_mapping = 0;

uint32_t mapped_in_segment = 0;

uint64_t last_page_used = 0;

struct memory_descrtiptor desc;

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
  } else {
    page_bitmap[byte] |= (1 << bit);
    last_page_used = page_index;
  }
}

// get a free page's index and set it to be used. see pp_alloc.
uint64_t get_free_page() {
  if (last_page_used >= MAX_PAGES) {
    last_page_used = MAX_PAGES - 1;
  }

  if (page_free(last_page_used + 1)) {
    set_page(last_page_used + 1, USED);
    return last_page_used;
  }

  // TODO: do this better!
  for (uint64_t i = 0; i <= MAX_PAGES; i++) {
    if (page_free(i)) {
      set_page(i, USED);
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

void *pp_alloc() {
  uint64_t index = get_free_page();

  struct page_mapping mapping;
  uint64_t prev_end;

retry:
  mapping = page_mappings[last_used_mapping];

  if (last_used_mapping != 0) {
    prev_end = page_mappings[last_used_mapping - 1].page_end_index;
  } else {
    prev_end = 0;
  }

  if (index >= mapping.page_start_index && index <= mapping.page_end_index) {
    k_debug("page %d allocated to %p.", index,
            ((index - prev_end) * PAGE_SIZE) + mapping.offset);

    return (void *)((index - prev_end) * PAGE_SIZE) + mapping.offset;

  } else if (last_used_mapping < desc.chunk_count) {
    last_used_mapping++;
    goto retry;
  }

  return NULL;
}

void setup_physical_paging() {
  desc = get_memory_descriptor();
  generate_page_mapping();

  for (uint32_t i = 0; i < desc.chunk_count; i++) {
    k_debug("pg mp %d: addr: %p st pg: %d end pg: %d", i,
            page_mappings[i].offset, page_mappings[i].page_start_index,
            page_mappings[i].page_end_index);
  }

  pp_alloc();
  last_page_used = 608;
  pp_alloc();
  last_page_used = 1048;
  pp_alloc();
  last_page_used = 77;
  pp_alloc();
  last_page_used = 56000;
  char *p = pp_alloc();
  p = "hello memory!";

  printf_("%s", p);
}
