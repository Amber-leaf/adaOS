#ifndef HEAP_H_
#define HEAP_H_

#include <stddef.h>

struct heap_free_block {
    size_t size;
    struct heap_free_block *next;
};

void setup_heap();
void heap_dump();
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t num, size_t size);
void *krealloc(void *ptr, size_t new_size);

#endif
