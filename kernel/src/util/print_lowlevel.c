#include "../header/core.h"
#include "../header/limine.h"
#include <stdbool.h>
#include <stdint.h>

#define MISSING font[0]

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint32_t on_colour = 0xffffff;
static uint32_t off_colour = 0x000000;
static uint8_t font_size = 2;
static uint8_t skew = 0;

static uint32_t cursor_x_max;
static uint32_t cursor_y_max;
static uint32_t bytes_per_line;
static uint32_t bytes_per_screen;
static uint32_t height;
static uint32_t width;
static uint32_t pitch;
static uint32_t *fb_ptr;

extern uint64_t font[128]; // From font.c

void calculate_screen_constants(struct limine_framebuffer *fb) {
  width = fb->width;
  height = fb->height;

  cursor_x_max = width / (8 * font_size) - 1;
  cursor_y_max = height / (8 * font_size) - 1;

  bytes_per_line = width * (8 * font_size) * 4;
  bytes_per_screen = bytes_per_line * (cursor_y_max + 1);

  pitch = fb->pitch / 4;

  fb_ptr = fb->address;
}

void scroll(uint8_t lines) {
  for (int n = 0; n < lines; n++) {
    for (uint32_t i = 0; i < cursor_y_max; i++) {
      uint32_t *dst = fb_ptr + (i * 8 * font_size) * pitch;
      uint32_t *src = fb_ptr + ((i + 1) * 8 * font_size) * pitch;
      memcpy(dst, src, width * 4 * 8 * font_size);
    }
    // Clear last line
    uint32_t *last = fb_ptr + (cursor_y_max * 8 * font_size) * pitch;
    memset(last, 0x00, width * 4 * 8 * font_size);
  }
}

void nl_cursor(void) {
  if (cursor_y + 1 > cursor_y_max) {
    scroll(1);
    cursor_x = 0;
  } else {
    cursor_x = 0;
    cursor_y++;
  }
}

void advance_cursor(void) {
  if (cursor_x + 1 > cursor_x_max) {
    nl_cursor();
  } else {
    cursor_x++;
  }
}

void cursor_goto(uint32_t x, uint32_t y) {
  if (x > cursor_x_max || y > cursor_y_max) {
    return;
  }

  cursor_x = x;
  cursor_y = y;
}

void print_bitmap(uint64_t bitmap, uint32_t x, uint32_t y) {
  for (int i = 0; i < 8; ++i) {
    uint8_t row = (bitmap >> ((7 - i) * 8)) & 0xFF;

    for (int n = 0; n < 8; ++n) {
      uint32_t color = (row & (1 << (7 - n))) ? on_colour : off_colour;

      for (int dy = 0; dy < font_size; ++dy) {
        for (int dx = 0; dx < font_size; dx++) {
          uint32_t px = x + n * font_size + dx + skew * (7 - i);
          uint32_t py = y + i * font_size + dy;
          fb_ptr[py * pitch + px] = color;
        }
      }
    }
  }
}

void set_text_colour(uint32_t colour) { on_colour = colour; }

void set_text_bg_colour(uint32_t colour) { off_colour = colour; }

void set_skew(uint8_t n) { skew = n; }

void clear(void) {
  memset(fb_ptr, 0x00, bytes_per_screen);

  cursor_x = cursor_y = 0;
}

uint32_t calculate_y(void) { return cursor_y * 8 * font_size; }

uint32_t calculate_x(void) { return cursor_x * 8 * font_size; }

void k_putc(uint16_t c) {
  bool should_print = true;

  switch (c) {
  case 0x0A:
    nl_cursor();
    __attribute__((fallthrough));
  case 0x0D:
    cursor_x = 0;
    should_print = false;
    break;
  }

  if (should_print) {
    uint64_t char_bitmap = font[c];
    if (c > sizeof(font) / (sizeof(font[0]) / 2))
      char_bitmap = MISSING;

    print_bitmap(char_bitmap, calculate_x(), calculate_y());

    advance_cursor();
  }
}

void k_puts(const char *s) {
  uint8_t i = 0;

  for (;;) {
    switch (s[i]) {
    case 0x00:
      return;
    case 0x0A:
      nl_cursor();
      __attribute__((fallthrough));
    case 0x0D:
      cursor_x = 0;
      break;
    default:
      k_putc(s[i]);
    }
    i++;
  }
}

void k_puti(uint32_t n) {
  uint32_t div = 1;
  uint32_t digit_count = 1;
  while (div <= n / 10) {
    digit_count++;
    div *= 10;
  }
  while (digit_count > 0) {
    uint32_t digit = (n / div) + 48;

    uint64_t char_bitmap = font[digit];
    if (digit > 57 || digit < 48)
      char_bitmap = MISSING;

    print_bitmap(char_bitmap, calculate_x(), calculate_y());

    advance_cursor();

    n %= div;
    div /= 10;
    digit_count--;
  }
}

void crlf(void) { k_puts("\r\n"); }