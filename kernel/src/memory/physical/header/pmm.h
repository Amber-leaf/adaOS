#ifndef PMM_H_
#define PMM_H_

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 1024 * 1024

void setup_pmm();
void *pp_alloc();
void *_pp_alloc(uint64_t page_index);
void* page_to_ptr(uint64_t page_index);
uint64_t ptr_to_page(void *ptr);
uint64_t get_free_page();
void set_page(uint64_t page_index, bool free);
bool page_free(uint64_t page_index);

struct page_mapping {
    uint64_t offset;
    uint64_t page_start_index;
    uint64_t page_end_index;
};

#endif
