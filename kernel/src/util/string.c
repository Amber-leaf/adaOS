#include "../header/core.h"
#include <stddef.h>

size_t strlen(const char *s) {
  for (size_t i = 0;; i++) {
    if (s[i] == 0x00) {
      return i;
    }
  }
  return -1;
}

char *stpcpy(char *restrict dst, const char *restrict src) {
  char *p;

  p = mempcpy(dst, src, strlen(src));
  *p = '\0';

  return p;
}

char *strcpy(char *restrict dst, const char *restrict src) {
  stpcpy(dst, src);
  return dst;
}

char *strcat(char *restrict dst, const char *restrict src) {
  stpcpy(dst + strlen(dst), src);
  return dst;
}