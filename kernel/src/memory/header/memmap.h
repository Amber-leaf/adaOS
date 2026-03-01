#ifndef MEMMAP_H_
#define MEMMAP_H_

#include <stdint.h>

#define MAX_CHUNKS 128

struct contiguous_memory_chunk {
    uint64_t base;
    uint64_t bounds;
};

struct memory_descrtiptor {
    uint64_t length;
    uint8_t chunk_count;
    struct contiguous_memory_chunk *chunk_ptr;
};

struct limine_memmap_response *get_memmap(void);
void print_free_ram();
void debug_print_mem_map();
struct memory_descrtiptor get_memory_descriptor();

#endif
