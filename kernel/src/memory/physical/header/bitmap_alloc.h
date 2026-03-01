#ifndef BITMAP_ALLOC_H_
#define BITMAP_ALLOC_H_

#include <stdint.h>

void setup_physical_paging();

struct page_mapping {
    uint32_t offset;
    uint64_t page_start_index;
    uint64_t page_end_index;
};

#endif
