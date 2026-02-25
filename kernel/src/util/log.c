#include "header/print_lowlevel.h"
#include "header/printf.h"

#define DEBUG
#define LOG
#define OK
#define ERR
#define WRN

void k_debug(char *format, ...) {
#ifdef DEBUG
  va_list va;
  va_start(va, format);

  set_text_colour(0x888888);
  k_puts("[DEBUG] Kernel: ");
  vprintf(format, va);
  va_end(va);
  set_text_colour(0xffffff);
  crlf();
#endif
}

void k_log(char *format, ...) {
#ifdef LOG
  va_list va;
  va_start(va, format);

  set_text_colour(0xffffff);
  k_puts("[LOG] Kernel: ");
  vprintf(format, va);
  va_end(va);
  crlf();
#endif
}

void k_ok(char *format, ...) {
#ifdef OK
  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(0x66ff66);
  k_puts("OK");
  set_text_colour(0xffffff);
  k_puts("] Kernel: ");

  vprintf(format, va);
  va_end(va);
  crlf();
#endif
}

void k_wrn(char *format, ...) {
#ifdef WRN
  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(0xffff66);
  k_puts("WRN");
  set_text_colour(0xffffff);
  k_puts("] Kernel: ");
  vprintf(format, va);
  va_end(va);
  crlf();
#endif
}

void k_err(char *format, ...) {
#ifdef ERR
  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(0xff6666);
  k_puts("ERR");
  set_text_colour(0xffffff);
  k_puts("] Kernel: ");
  vprintf(format, va);
  va_end(va);
  crlf();
#endif
}