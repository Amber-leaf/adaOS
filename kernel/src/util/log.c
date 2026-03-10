#include "../platform/x86_64//header/pit.h"
#include "header/print_lowlevel.h"
#include "header/printf.h"

#define PLAIN_TEXT_COLOUR 0xb0b0b0

#define DBG_TEXT_COLOUR 0x888888
#define OK_TEXT_COLOUR 0x66ff66
#define WRN_TEXT_COLOUR 0xffff66
#define ERR_TEXT_COLOUR 0xff6666

// #define DBG
#define LOG
#define OK
#define WRN
#define ERR

#define TEST_PASS
#define TEST_FAIL

#define TEST_FAIL_FREQUENCY 400
#define TEST_FAIL_HOLD 400

void k_debug(char *format, ...) {
#ifdef DBG
  va_list va;
  va_start(va, format);

  set_text_colour(DBG_TEXT_COLOUR);
  k_puts("[DBG] Kernel: ");
  vprintf(format, va);
  va_end(va);
  set_text_colour(PLAIN_TEXT_COLOUR);
  crlf();
#endif
}

void k_log(char *format, ...) {
#ifdef LOG
  va_list va;
  va_start(va, format);

  set_text_colour(PLAIN_TEXT_COLOUR);
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
  set_text_colour(OK_TEXT_COLOUR);
  k_puts("OK~");
  set_text_colour(PLAIN_TEXT_COLOUR);
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
  set_text_colour(WRN_TEXT_COLOUR);
  k_puts("WRN");
  set_text_colour(PLAIN_TEXT_COLOUR);
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
  set_text_colour(ERR_TEXT_COLOUR);
  k_puts("ERR");
  set_text_colour(PLAIN_TEXT_COLOUR);
  k_puts("] Kernel: ");
  vprintf(format, va);
  va_end(va);
  crlf();
#endif
}

void k_test_pass(char *format, ...) {
#ifdef TEST_PASS
  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(OK_TEXT_COLOUR);
  k_puts("TEST PASS");
  set_text_colour(PLAIN_TEXT_COLOUR);
  k_puts("] ");
  vprintf(format, va);
  va_end(va);
  crlf();
#endif
}

void k_test_fail(char *format, ...) {
#ifdef TEST_FAIL
  if (get_pit_configured()) {
    play_sound(TEST_FAIL_FREQUENCY);
  }

  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(ERR_TEXT_COLOUR);
  k_puts("TEST FAIL");
  set_text_colour(PLAIN_TEXT_COLOUR);
  k_puts("] ");
  vprintf(format, va);
  va_end(va);
  crlf();

  if (get_pit_configured()) {
    pit_sleep_ms(TEST_FAIL_HOLD);
    sound_off();
  }
#endif
}