#ifndef CORE_H_   /* Include guard */
#define CORE_H_
#include "limine.h"
#include <stdint.h>
void k_putc(uint16_t c);
void k_puts(const char *s);
void k_puti(uint32_t n);
void crlf(void);
void set_text_bg_colour(uint32_t colour);
void set_text_colour(uint32_t colour);
void scroll(uint8_t lines);
void hcf(void);
#endif