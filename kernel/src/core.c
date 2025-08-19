#include "core.h"
#include "log.h"
#include "printf.h"

#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((
    used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used,
               section(".limine_requests_"
                       "start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used,
    section(
        ".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
  uint8_t *restrict pdest = (uint8_t *restrict)dest;
  const uint8_t *restrict psrc = (const uint8_t *restrict)src;

  for (size_t i = 0; i < n; i++) {
    pdest[i] = psrc[i];
  }

  return dest;
}

void *memset(void *s, int c, size_t n) {
  uint8_t *p = (uint8_t *)s;

  for (size_t i = 0; i < n; i++) {
    p[i] = (uint8_t)c;
  }

  return s;
}

void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  if (src > dest) {
    for (size_t i = 0; i < n; i++) {
      pdest[i] = psrc[i];
    }
  } else if (src < dest) {
    for (size_t i = n; i > 0; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
  }

  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return p1[i] < p2[i] ? -1 : 1;
    }
  }

  return 0;
}

// ---------- Kernel Start + Core Functions ----------

static char *version_string = "v0.0.1";

static uint32_t cursorx = 0;
static uint32_t cursory = 0;
static uint32_t on_colour = 0xffffff;
static uint32_t off_colour = 0x000000;
static uint8_t font_size = 2;
static uint8_t skew = 0;

static uint32_t cursorx_max;
static uint32_t cursory_max;
static uint32_t bytes_per_line;
static uint32_t bytes_per_screen;
static uint32_t height;
static uint32_t width;
static uint32_t pitch;
static uint32_t *fb_ptr;

extern uint64_t font[128]; // From font.c

// Halt and catch fire function.
static void hcf(void) {
  for (;;) {
    asm("hlt");
  }
}

// === Printing / Frambuffer ===

static struct limine_framebuffer *get_framebuffer(void) {
  // Ensure we got a framebuffer.
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    hcf();
  }

  return framebuffer_request.response->framebuffers[0];
}

void calculate_screen_constants(void) {
  struct limine_framebuffer *fb = get_framebuffer();

  width = fb->width;
  height = fb->height;

  cursorx_max = width / (8 * font_size) - 1;
  cursory_max = height / (8 * font_size) - 1;

  bytes_per_line = width * (8 * font_size);
  bytes_per_screen = bytes_per_line * cursory_max;

  pitch = fb->pitch / 4;

  fb_ptr = fb->address;
}

void nl_cursor(void) {
  if (cursory + 1 > cursory_max) {
    scroll(1);
    cursorx = 0;
  } else {
    cursorx = 0;
    cursory++;
  }
}

void advance_cursor(void) {
  if (cursorx + 1 > cursorx_max) {
    nl_cursor();
  } else {
    cursorx++;
  }
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

  cursorx = cursory = 0;
}

uint32_t calculate_y(void) { return cursory * 8 * font_size; }

uint32_t calculate_x(void) { return cursorx * 8 * font_size; }

void k_putc(uint16_t c) {
  uint64_t char_bitmap = font[c];
  if (c > sizeof(font) / (sizeof(font[0]) / 2))
    char_bitmap = font[1]; // Missing char

  print_bitmap(char_bitmap, calculate_x(), calculate_y());

  advance_cursor();
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
      cursorx = 0;
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
      char_bitmap = font[0]; // Missing char

    print_bitmap(char_bitmap, calculate_x(), calculate_y());

    advance_cursor();

    n %= div;
    div /= 10;
    digit_count--;
  }
}

void crlf(void) { k_puts("\r\n"); }

void scroll(uint8_t lines) {
  uint32_t *ptr = fb_ptr;

  for (int n = 0; n < lines; n++) {
    for (uint32_t i = 0; i < cursory_max; i++) {
      memcpy(ptr, ptr + bytes_per_line, width * 4 * 8 * font_size);

      ptr += bytes_per_line;
    }

    memset(ptr, off_colour, width * 4 * 8 * font_size);

    ptr = fb_ptr;
  }
}

void print_banner(void) {
  set_text_colour(0xe6a6a1);
  set_skew(1);
  k_putc(205);
  printf("adaOS, %s", version_string);
  k_putc(205);
  set_skew(0);

  set_text_colour(0xffffff);
  crlf();
  k_puts("Copyright (C) 2025 Isabelle M. S.");
  crlf();
  crlf();
}

// === Entering Long Mode ===
extern uint8_t check_CPUID(void);     // From bootstap_longmode.asm
extern uint8_t query_long_mode(void); // From bootstap_longmode.asm
extern void set_paging(void);         // From bootstap_longmode.asm
extern void set_compatibility(void);  // From bootstap_longmode.asm

void kmain(void) {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    hcf();
  }

  calculate_screen_constants();
  print_banner();

  if (!check_CPUID()) {
    k_err("CPUID not supported!");
    hcf();
  }

  if (!query_long_mode()) {
    k_err("long mode not supported!");
    hcf();
  }

  set_paging();
  set_compatibility();

  k_ok("in 32-bit compatibility mode");

  hcf();
}
