// Adapted from the Arikoto Operating System Development Project;
// https://codeberg.org/NerdNextDoor/arikoto.

#include "header/heap.h"

#include <stdint.h>

#include "../header/core.h"
#include "../util/header/log.h"
#include "../util/header/panic.h"
#include "./virtual/header/vmm.h"
#include "header/memmap.h"
#include "physical/header/pmm.h"

#define MIN_ALLOC_SIZE sizeof(struct heap_free_block)
#define HEAP_ALIGNMENT 16

#define ALIGN_UP_HEAP(size)                                                    \
  (((size) + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1))

#define KERNEL_HEAP_START HIGHER_HALF + 0x10000000000
#define INITIAL_HEAP_PAGES 256
#define KERNEL_HEAP_INITIAL_SIZE (INITIAL_HEAP_PAGES * PAGE_SIZE)

void *heap_start = NULL;
size_t heap_size = 0;
heap_free_block_t *free_list_head = NULL;

extern pagemap_t *kernel_pagemap;

int heap_grow_pages(size_t pages) {
  if (pages == 0)
    return 0;

  uintptr_t base = (uintptr_t)heap_start + heap_size;
  uintptr_t new_region_size = pages * PAGE_SIZE;

  for (size_t i = 0; i < pages; ++i) {
    void *phys = pp_alloc();
    if (!phys) {
      k_err("heap_expand_pages: pp_alloc failed at page %lx!", i);
      return 0;
    }

    uintptr_t virt = base + (i * PAGE_SIZE);
    if (!map_page(kernel_pagemap, virt, (uintptr_t)phys,
                  PTE_PRESENT | PTE_WRITABLE | PTE_NX)) {
      panic("heap_expand_pages: vmm_map_page failed");
      return 0;
    }
  }

  heap_free_block_t *new_block = (heap_free_block_t *)base;
  new_block->size = new_region_size;
  new_block->next = NULL;

  if (!free_list_head) {
    free_list_head = new_block;
  } else {
    heap_free_block_t *next_block = NULL;
    heap_free_block_t *head_block = free_list_head;
    while (head_block && (uintptr_t)head_block < base) {
      next_block = head_block;
      head_block = head_block->next;
    }
    if (next_block == NULL) {
      new_block->next = free_list_head;
      free_list_head = new_block;
    } else {
      new_block->next = next_block->next;
      next_block->next = new_block;
    }

    if (new_block->next && ((uintptr_t)new_block + new_block->size) ==
                               (uintptr_t)new_block->next) {
      new_block->size += new_block->next->size;
      new_block->next = new_block->next->next;
    }
    if (next_block &&
        ((uintptr_t)next_block + next_block->size) == (uintptr_t)new_block) {
      next_block->size += new_block->size;
      next_block->next = new_block->next;
    }
  }

  heap_size += new_region_size;
  return 1;
}

void heap_dump(void) {
  k_debug("Heap dump: start=%p size=%lX free-list:", (uint64_t)heap_start,
          heap_size);
  for (heap_free_block_t *b = free_list_head; b; b = b->next) {
    k_debug("block %lX size=%lX next=%p", (uint64_t)b, b->size, b->next);
  }
}

void *kmalloc(size_t size) {
  if (size == 0)
    return NULL;
  size_t payload = ALIGN_UP_HEAP(size);
  size_t header_sz = ALIGN_UP_HEAP(sizeof(size_t));
  size_t total_size = payload + header_sz;

  if (total_size < MIN_ALLOC_SIZE)
    total_size = MIN_ALLOC_SIZE;

  heap_free_block_t *previous_block = NULL;
  heap_free_block_t *current_block = free_list_head;

retry_search:
  while (current_block) {
    if (current_block->size >= total_size) {
      if (current_block->size >= total_size + MIN_ALLOC_SIZE) {
        heap_free_block_t *new_block =
            (heap_free_block_t *)((uintptr_t)current_block + total_size);
        new_block->size = current_block->size - total_size;
        new_block->next = current_block->next;

        current_block->size = total_size;

        if (previous_block == NULL) {
          free_list_head = new_block;
        } else {
          previous_block->next = new_block;
        }
      } else {
        if (previous_block == NULL) {
          free_list_head = current_block->next;
        } else {
          previous_block->next = current_block->next;
        }
      }

      size_t *size_ptr = (size_t *)current_block;
      *size_ptr = current_block->size;

      void *ptr = (void *)((uintptr_t)current_block + header_sz);
      return ptr;
    }
    previous_block = current_block;
    current_block = current_block->next;
  }

  if (!heap_grow_pages(16)) {
    if (!heap_grow_pages(1)) {
      k_err("kmalloc: Out of heap memory (requested %lx bytes)", size);
      heap_dump();
      return NULL;
    }
  }
  previous_block = NULL;
  current_block = free_list_head;
  goto retry_search;
}

void kfree(void *ptr) {
  if (!ptr)
    return;

  size_t header_sz = ALIGN_UP_HEAP(sizeof(size_t));
  size_t *size_ptr = (size_t *)((uintptr_t)ptr - header_sz);
  void *block_start = (void *)size_ptr;
  size_t block_size = *size_ptr;

  if ((uintptr_t)block_start < (uintptr_t)heap_start ||
      (uintptr_t)block_start >= (uintptr_t)heap_start + heap_size) {
    panic("kfree: invalid pointer (out of heap range).");
    return;
  }
  if (block_size < MIN_ALLOC_SIZE) {
    panic("kfree: invalid block size.");
    return;
  }
  if (((uintptr_t)block_start & (HEAP_ALIGNMENT - 1)) != 0) {
    panic("kfree: alignment error. (try calling HEAP_ALIGN_UP(ptr))");
    return;
  }

  heap_free_block_t *previous_block = NULL;
  heap_free_block_t *current_block = free_list_head;
  while (current_block && (uintptr_t)current_block < (uintptr_t)block_start) {
    previous_block = current_block;
    current_block = current_block->next;
  }

  heap_free_block_t *freed_block = (heap_free_block_t *)block_start;
  freed_block->size = block_size;

  if (previous_block == NULL) {
    freed_block->next = free_list_head;
    free_list_head = freed_block;
  } else {
    freed_block->next = previous_block->next;
    previous_block->next = freed_block;
  }

  if (freed_block->next && ((uintptr_t)freed_block + freed_block->size) ==
                               (uintptr_t)freed_block->next) {
    freed_block->size += freed_block->next->size;
    freed_block->next = freed_block->next->next;
  }
  if (previous_block && ((uintptr_t)previous_block + previous_block->size) ==
                            (uintptr_t)freed_block) {
    previous_block->size += freed_block->size;
    previous_block->next = freed_block->next;
  }
}

void *kcalloc(size_t num, size_t size) {
  if (size != 0 && num > (SIZE_MAX / size))
    return NULL;

  size_t total = num * size;
  void *p = kmalloc(total);

  if (p)
    memset(p, 0, total);
  return p; // Will be NULL if size = 0 (kmalloc(0) -> NULL)
}

void *krealloc(void *ptr, size_t new_size) {
  if (!ptr)
    return kmalloc(new_size);

  if (new_size == 0) {
    kfree(ptr);
    return NULL;
  }

  size_t header_sz = ALIGN_UP_HEAP(sizeof(size_t));
  size_t old_total = *((size_t *)((uintptr_t)ptr - header_sz));
  size_t old_payload = (old_total >= header_sz) ? (old_total - header_sz) : 0;
  if (new_size <= old_payload)
    return ptr;

  void *new_ptr = kmalloc(new_size);

  if (!new_ptr)
    return NULL;

  memcpy(new_ptr, ptr, old_payload);
  kfree(ptr);

  return new_ptr;
}

void setup_heap() {
  k_debug("heap start at %p", KERNEL_HEAP_START);

  heap_start = (void *)KERNEL_HEAP_START;
  heap_size = 0;

  if (!heap_grow_pages(INITIAL_HEAP_PAGES)) {
    panic("setup_heap: failed to allocate initial kernel heap!");
  }

  free_list_head = (heap_free_block_t *)heap_start;
  free_list_head->size = heap_size;
  free_list_head->next = NULL;
}