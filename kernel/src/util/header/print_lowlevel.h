#ifndef PRINT_LOWLEVEL_H_
#define PRINT_LOWLEVEL_H_

#include "../../header/limine.h"
#include <stdint.h>

void calculate_screen_constants(struct limine_framebuffer *fb);
void scroll(uint8_t lines);
void nl_cursor(void);
void advance_cursor(void);
void print_bitmap(uint64_t bitmap, uint32_t x, uint32_t y);
void set_text_colour(uint32_t colour);
void set_text_bg_colour(uint32_t colour);
void set_skew(uint8_t n);
void clear(void);
uint32_t calculate_y(void);
uint32_t calculate_x(void);
void k_putc(uint16_t c);
void k_puts(const char *s);
void k_puti(uint32_t n);
void crlf(void);

#endif