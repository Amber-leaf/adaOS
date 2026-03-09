#ifndef HEAP_H_
#define HEAP_H_

#include <stddef.h>

struct heap_free_block {
    size_t size;
    struct heap_free_block *next;
};

void setup_heap();

#endif
