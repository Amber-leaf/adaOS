#ifndef CORE_H_   /* Include guard */
#define CORE_H_

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void hcf(void);
struct limine_framebuffer *get_framebuffer(void);
int64_t get_boot_time(void);

#endif