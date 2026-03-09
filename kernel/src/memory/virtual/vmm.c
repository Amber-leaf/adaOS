// Adapted from the Arikoto Operating System Development Project;
// https://codeberg.org/NerdNextDoor/arikoto.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../header/core.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"
#include "../header/memmap.h"
#include "../physical/header/pmm.h"
#include "header/vmm.h"

extern uint8_t _text_start[], _text_end[];
extern uint8_t _rodata_start[], _rodata_end[];
extern uint8_t _data_start[], _data_end[];
extern uint8_t _bss_start[], _bss_end[];

pagemap_t *kernel_pagemap = NULL;

static bool is_table_empty(uintptr_t *table_virt) {
  for (int i = 0; i < 512; i++) {
    if (table_virt[i] != 0) {
      return false;
    }
  }
  return true;
}

uintptr_t *get_next_level(uintptr_t *current_level_virt, size_t index) {
  uintptr_t entry = current_level_virt[index];

  if (entry & PTE_PRESENT) {
    return (uintptr_t *)(PTE_GET_ADDR(entry) + HIGHER_HALF);
  }

  k_wrn("No next paging level, but was asked to get it anyway!");
  return NULL;
}

uintptr_t *allocate_next_level(uintptr_t *current_level_virt, size_t index,
                               uint64_t pte_flags) {

  void *next_level_phys = pp_alloc();

  if (next_level_phys == NULL) {
    k_err("Failed to allocate page for new page table level (index %u)!",
          (unsigned)index);
    return NULL;
  }

  uint64_t new_entry_flags =
      pte_flags ? pte_flags : (PTE_PRESENT | PTE_WRITABLE);

  current_level_virt[index] =
      (uint64_t)(uintptr_t)next_level_phys | new_entry_flags;

  uint64_t *next_level_virt =
      (uint64_t *)((uintptr_t)next_level_phys + HIGHER_HALF);

  memset(next_level_virt, 0, PAGE_SIZE);

  return next_level_virt;
}

uintptr_t *get_or_allocate_next_level(uintptr_t *level, size_t index,
                                      uint64_t flags) {
  if (level[index] & PTE_PRESENT)
    return (uintptr_t *)(PTE_GET_ADDR(level[index]) + HIGHER_HALF);

  return allocate_next_level(level, index, flags);
}

bool map_page(uintptr_t virt_addr, uintptr_t phys_addr, uint64_t flags) {
  virt_addr &= ~(PAGE_SIZE - 1);
  phys_addr &= ~(PAGE_SIZE - 1);

  size_t pml4_index = (virt_addr >> 39) & 0x1FF;
  size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
  size_t pd_index = (virt_addr >> 21) & 0x1FF;
  size_t pt_index = (virt_addr >> 12) & 0x1FF;

  uint64_t alloc_flags = PTE_PRESENT | PTE_WRITABLE;

  if (flags & PTE_USER)
    alloc_flags |= PTE_USER;

  uint64_t *pml4 = kernel_pagemap->top_level;

  uint64_t *pdpt = get_or_allocate_next_level(pml4, pml4_index, alloc_flags);
  if (!pdpt)
    goto fail;
  uint64_t *pd = get_or_allocate_next_level(pdpt, pdpt_index, alloc_flags);
  if (!pd)
    goto fail;
  uint64_t *pt = get_or_allocate_next_level(pd, pd_index, alloc_flags);
  if (!pt)
    goto fail;

  pt[pt_index] = phys_addr | flags | PTE_PRESENT;

  // Flush the changes.
  asm volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");

  return true;

fail:
  k_err("Failed to map virtual addr %p -> physical addr %p!",
        (uintptr_t *)virt_addr, (uintptr_t *)phys_addr);
  return false;
}

bool unmap_page(pagemap_t *pagemap, uintptr_t virt_addr) {
  if (virt_addr % PAGE_SIZE != 0) {
    k_err("unmap_page called with non-aligned virt %p", (void *)virt_addr);
    return false;
  }

  size_t pml4_index = (virt_addr >> 39) & 0x1FF;
  size_t pdpt_index = (virt_addr >> 30) & 0x1FF;
  size_t pd_index = (virt_addr >> 21) & 0x1FF;
  size_t pt_index = (virt_addr >> 12) & 0x1FF;

  uint64_t *pml4 = pagemap->top_level;

  uint64_t *pdpt = get_next_level(pml4, pml4_index);
  if (!pdpt) {
    return true;
  }
  uint64_t pdpt_entry = pml4[pml4_index];

  uint64_t *pd = get_next_level(pdpt, pdpt_index);
  if (!pd) {
    return true;
  }
  uint64_t pd_entry = pdpt[pdpt_index];

  uint64_t *pt = get_next_level(pd, pd_index);
  if (!pt) {
    return true;
  }
  uint64_t pt_entry = pd[pd_index];

  if (!(pt[pt_index] & PTE_PRESENT)) {
    return true;
  }

  pt[pt_index] = 0;

  asm volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");

  if (is_table_empty(pt)) {
    pd[pd_index] = 0;

    uintptr_t pt_phys = PTE_GET_ADDR(pt_entry);
    pp_free((void *)pt_phys);

    if (is_table_empty(pd)) {
      pdpt[pdpt_index] = 0;

      uintptr_t pd_phys = PTE_GET_ADDR(pd_entry);
      pp_free((void *)pd_phys);

      if (is_table_empty(pdpt)) {
        if (pml4_index >= 256) {
          return true;
        }
        pml4[pml4_index] = 0;
        uintptr_t pdpt_phys = PTE_GET_ADDR(pdpt_entry);
        pp_free((void *)pdpt_phys);
      }
    }
  }

  return true;
}

void switch_to_pagemap(pagemap_t *pagemap) {
  if (!pagemap || !pagemap->top_level) {
    panic("Attempted to switch to an invalid pagemap\n");
    return;
  }

  uintptr_t pml4_phys = (uintptr_t)pagemap->top_level - HIGHER_HALF;

  asm volatile("mov %0, %%cr3" ::"r"(pml4_phys) : "memory");
}

void setup_vmm() {
  k_debug("seting up vmm");

  void *pml4_phys = pp_alloc();

  if (pml4_phys == NULL) {
    panic("Failed to allocate kernel PML4 table page.");
  }

  uint64_t *pml4_virt = (uint64_t *)((uintptr_t)pml4_phys + HIGHER_HALF);

  memset(pml4_virt, 0, PAGE_SIZE);

  static pagemap_t k_pagemap;
  kernel_pagemap = &k_pagemap;
  kernel_pagemap->top_level = pml4_virt;

  for (int i = 256; i <= 511; i++) {
    get_or_allocate_next_level(pml4_virt, i, 0);
  }

  uintptr_t kernel_virt_end = ALIGN_UP((uintptr_t)_bss_end, PAGE_SIZE);

  for (uintptr_t p_virt = get_k_addr()->virtual_base; p_virt < kernel_virt_end;
       p_virt += PAGE_SIZE) {
    uintptr_t p_phys =
        (p_virt - get_k_addr()->virtual_base) + get_k_addr()->physical_base;
    uint64_t flags = PTE_PRESENT;

    if (p_virt >= (uintptr_t)_text_start && p_virt < (uintptr_t)_text_end) {

    } else if (p_virt >= (uintptr_t)_rodata_start &&
               p_virt < (uintptr_t)_rodata_end) {
      flags |= PTE_NX;
    } else if (p_virt >= (uintptr_t)_data_start &&
               p_virt < (uintptr_t)_data_end) {
      flags |= PTE_WRITABLE | PTE_NX;
    } else {
      flags |= PTE_WRITABLE | PTE_NX;
    }

    if (!map_page(p_virt, p_phys, flags)) {
      panic("Failed to map kernel page.");
    }
  }

  k_debug("kernel mapped %p - %p", get_k_addr()->virtual_base, kernel_virt_end);

  memory_descriptor_t desc = get_memory_descriptor();

  uintptr_t min = UINTPTR_MAX;
  uintptr_t max = 0;

  for (size_t i = 0; i < desc.chunk_count; i++) {
    contiguous_memory_chunk_t entry = desc.chunk_ptr[i];

    uintptr_t base = entry.base;
    if (base < min)
      min = base;

    uintptr_t top = entry.bounds;
    if (top > max)
      max = top;

    uintptr_t map_base = ALIGN_UP(base, PAGE_SIZE);
    uintptr_t map_top = ALIGN_UP(top, PAGE_SIZE);

    if (map_top <= map_base)
      continue;

    for (uintptr_t p = map_base; p < map_top; p += PAGE_SIZE) {
      if (!map_page(p + HIGHER_HALF, p, PTE_PRESENT | PTE_WRITABLE | PTE_NX)) {
        panic("Failed to map HHDM page.");
      }
    }
  }

  k_debug("HHDM mapped %p - %p", min, max);

  struct limine_framebuffer *fb = get_framebuffer();

  uint32_t total_bytes = fb->pitch * fb->height;
  size_t pages = (total_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

  uintptr_t virt_addr = (uintptr_t)fb->address;
  uintptr_t phys_addr = ((uintptr_t)fb->address - HIGHER_HALF);

  for (size_t i = 0; i < pages; i++) {
    map_page(virt_addr, phys_addr,
             PTE_PRESENT | PTE_WRITABLE | PTE_PCD | PTE_PAT);

    asm volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");

    virt_addr += PAGE_SIZE;
    phys_addr += PAGE_SIZE;
  }

  k_debug("framebuffer mapped: %p - %p", fb->address, virt_addr);

  switch_to_pagemap(kernel_pagemap);
}
