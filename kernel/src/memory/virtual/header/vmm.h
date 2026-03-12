#ifndef VMM_H_
#define VMM_H_

#include <stdint.h>
#include <stdbool.h>

// Page flags.
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER (1ull << 2)
#define PTE_PWT (1ull << 3)
#define PTE_PCD (1ull << 4)
#define PTE_ACCESSED (1ull << 5)
#define PTE_DIRTY (1ull << 6)
#define PTE_PAT (1ull << 7)
#define PTE_GLOBAL (1ull << 8)
#define PTE_NX (1ull << 63)

// Masks.
#define PTE_ADDR_MASK 0x000ffffffffff000
#define PTE_GET_ADDR(VALUE) ((VALUE) & PTE_ADDR_MASK)
#define PTE_GET_FLAGS(VALUE) ((VALUE) & ~PTE_ADDR_MASK)

typedef struct pagemap {
  uint64_t *top_level;
} pagemap_t;

void setup_vmm();
bool map_page(pagemap_t* pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags);
void switch_to_pagemap(pagemap_t *pagemap);

#endif
