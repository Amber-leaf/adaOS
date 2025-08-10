#ifndef FOO_H_   /* Include guard */
#define FOO_H_
#include <stdint.h>
void putc(uint8_t c);
void puts(const char *s);
void puti(uint32_t n);
void crnl(void);
void set_text_bg_colour(uint32_t colour);
void set_text_colour(uint32_t colour);
void scroll(uint8_t lines);

#endif