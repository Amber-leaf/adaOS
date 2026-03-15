#ifndef STRING_H_
#define STRING_H_

#include <stddef.h>
size_t strlen(const char* s);

char *stpcpy(char *restrict dst, const char *restrict src);

char *strcpy(char *restrict dst, const char *restrict src);

char *strcat(char *restrict dst, const char *restrict src);

#endif