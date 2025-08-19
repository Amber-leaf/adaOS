#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core.h"
#include "log.h"

static uint64_t kernel_base;
static uint64_t kernel_size;

__attribute__((aligned(0x1000))) static uint64_t pml4[512];
__attribute__((aligned(0x1000))) static uint64_t pdpt[512];
__attribute__((aligned(0x1000))) static uint64_t pd_identity[512];
__attribute__((aligned(0x1000))) static uint64_t pd_higherhalf[512];

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_LARGE 0x80
#define PAGE_USER 0x4

#define PAGE_SIZE 0x1000
#define LARGE_PAGE_SIZE 0x200000 // 2 MiB
#define PML4_ENTRIES 512

#define KERNEL_VMA 0xFFFFFFFF80000000

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_executable_file_request
    executable_file_request = {.id = LIMINE_EXECUTABLE_FILE_REQUEST,
                               .revision = 0};

void get_k_info(void) {
  if (executable_file_request.response == NULL ||
      executable_file_request.response->executable_file == NULL) {
    hcf();
  }

  struct limine_file *kernel_file =
      executable_file_request.response->executable_file;
  kernel_base = (uint64_t)kernel_file->address;
  kernel_size = kernel_file->size;

#define PHYS_BASE kernel_base
#define MAP_SIZE kernel_size

  k_debug("base: %x", kernel_base);
  k_debug("vbase: %x", kernel_base + KERNEL_VMA);
  k_debug("size: %x", kernel_size);
}

void setup_paging(void) {
  get_k_info();

  pd_identity[0] = (0x000000) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_LARGE;

  k_debug("1");

  // Map higher-half virtual address to same physical region
  size_t higher_half_index = (KERNEL_VMA >> 21) & 0x1FF;
  pd_higherhalf[higher_half_index] =
      PHYS_BASE | PAGE_PRESENT | PAGE_WRITABLE | PAGE_LARGE;

  k_debug("2");

  pdpt[0] = ((uint64_t)pd_identity) | PAGE_PRESENT | PAGE_WRITABLE;
  pdpt[1] = ((uint64_t)pd_higherhalf) | PAGE_PRESENT | PAGE_WRITABLE;

  pml4[0] = ((uint64_t)pdpt) | PAGE_PRESENT | PAGE_WRITABLE;
  pml4[511] = ((uint64_t)pdpt) | PAGE_PRESENT | PAGE_WRITABLE;

  k_debug("3");

#define PHYS(x) (KERNEL_VMA - (uintptr_t)(x))
  asm volatile("mov %0, %%cr3" ::"r"(PHYS(pml4)) : "memory");

  k_debug("finished paging");
}
