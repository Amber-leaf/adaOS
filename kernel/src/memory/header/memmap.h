#ifndef MEMMAP_H_
#define MEMMAP_H_

#include <stdint.h>
#include "../../header/limine.h"

#define PAGE_SIZE 4096

#define MAX_PAGES 1024 * 1024

#define HIGHER_HALF (get_hhdm()->offset)

#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

#define MAX_SEGMENTS 32
#define MAX_CHUNKS 128

typedef struct contiguous_memory_chunk {
    uint64_t base;
    uint64_t bounds;
} contiguous_memory_chunk_t;

typedef struct memory_descriptor {
    uint64_t length;
    uint8_t chunk_count;
    struct contiguous_memory_chunk *chunk_ptr;
} memory_descriptor_t;

void print_free_ram();
void debug_print_mem_map();
void serial_print_mem_map();

memory_descriptor_t get_memory_descriptor();

struct limine_memmap_response *get_memmap(void);
struct limine_executable_address_response *get_k_addr(void);
struct limine_hhdm_response *get_hhdm(void);
struct limine_executable_file_response *get_exe(void);

#endif
