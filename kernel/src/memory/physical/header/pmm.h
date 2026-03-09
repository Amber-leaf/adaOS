#ifndef PMM_H_
#define PMM_H_

#include <stdint.h>
#include <stdbool.h>

void setup_pmm();

void *pp_alloc();
void *_pp_alloc(uint64_t page_index);

void pp_free(void *ptr);
void _pp_free(uint64_t page_index);

void* page_to_ptr(uint64_t page_index);
uint64_t ptr_to_page(void *ptr);

uint64_t get_free_page();

typedef struct page_mapping {
    uint64_t offset;
    uint64_t page_start_index;
    uint64_t page_end_index;
} page_mapping_t;

#endif
