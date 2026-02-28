#ifndef MEMMAP_H_
#define MEMMAP_H_

#include <stdint.h>

struct contiguous_memory_chunk {
    uint64_t base;
    uint64_t bounds;
};



struct limine_memmap_response *get_memmap(void);
void print_free_ram();

#endif
