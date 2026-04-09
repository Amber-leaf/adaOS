#include "../platform/x86_64/header/apic.h"
#include "../platform/x86_64/header/cpu.h"
#include "../platform/x86_64/header/pic.h"
#include "../platform/x86_64/header/pit.h"

#include "../scheduler/header/scheduler.h"
#include "../scheduler/header/spinlock.h"

#include "header/print_lowlevel.h"
#include "header/printf.h"
#include "header/state.h"
#include <stdbool.h>
#include <stdint.h>

#define PLAIN_TEXT_COLOUR 0xb0b0b0

#define DBG_TEXT_COLOUR 0x888888
#define OK_TEXT_COLOUR 0x66ff66
#define WRN_TEXT_COLOUR 0xffff66
#define ERR_TEXT_COLOUR 0xff6666

#define DBG
#define LOG
#define OK
#define WRN
#define ERR

#define TODO

#define TEST_PASS
#define TEST_FAIL

#define TEST_FAIL_FREQUENCY 400
#define TEST_FAIL_HOLD 400

SPINLOCK_DEFINE(log_lock);

void k_debug(char *format, ...) {
#ifdef DBG
  spinlock_acquire(&log_lock);
  serial_printf("debug\n");

  char buffer[256];

  va_list va;
  va_start(va, format);

  vsnprintf_(buffer, 256, format, va);
  serial_printf_("%s\n", buffer);

  //
  // set_text_colour(DBG_TEXT_COLOUR);
  //
  // if (get_global_state().smp_initialized) {
  //  printf_("(CPU %d) ", (uint64_t)get_cpu()->id);
  //}
  //
  // k_puts("[DBG] Kernel: ");
  // vprintf_(format, va);
  // va_end(va);
  // set_text_colour(PLAIN_TEXT_COLOUR);
  // crlf();

  serial_printf("debug done\n");

  spinlock_release(&log_lock);
#endif
}

void k_log(char *format, ...) {
#ifdef LOG
  spinlock_acquire(&log_lock);
  va_list va;
  va_start(va, format);

  set_text_colour(PLAIN_TEXT_COLOUR);
  k_puts("[LOG] Kernel: ");
  vprintf(format, va);
  va_end(va);
  crlf();
  spinlock_release(&log_lock);
#endif
}

void k_ok(char *format, ...) {
#ifdef OK
  spinlock_acquire(&log_lock);
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
  spinlock_release(&log_lock);
#endif
}

void k_wrn(char *format, ...) {
#ifdef WRN
  spinlock_acquire(&log_lock);
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
  spinlock_release(&log_lock);
#endif
}

void k_err(char *format, ...) {
#ifdef ERR
  spinlock_acquire(&log_lock);
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
  spinlock_release(&log_lock);
#endif
}

void k_todo(char *format, ...) {
#ifdef TODO
  spinlock_acquire(&log_lock);
  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(DBG_TEXT_COLOUR);
  k_puts("   TODO~   ");
  set_text_colour(PLAIN_TEXT_COLOUR);
  k_puts("] ");
  vprintf(format, va);
  va_end(va);
  crlf();
  spinlock_release(&log_lock);
#endif
}

void k_test_pass(char *format, ...) {
#ifdef TEST_PASS
  spinlock_acquire(&log_lock);
  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(OK_TEXT_COLOUR);
  k_puts("TEST PASSED");
  set_text_colour(PLAIN_TEXT_COLOUR);
  k_puts("] ");
  vprintf(format, va);
  va_end(va);
  crlf();
  spinlock_release(&log_lock);
#endif
}

void k_test_fail(char *format, ...) {
#ifdef TEST_FAIL
  spinlock_acquire(&log_lock);
  if (get_global_state().pit_initialized) {
    play_sound(TEST_FAIL_FREQUENCY);
  }

  va_list va;
  va_start(va, format);

  k_puts("[");
  set_text_colour(ERR_TEXT_COLOUR);
  k_puts("TEST FAILED");
  set_text_colour(PLAIN_TEXT_COLOUR);
  k_puts("] ");
  vprintf(format, va);
  va_end(va);
  crlf();

  if (get_global_state().pit_initialized) {
    if (get_global_state().pit_timer_running) {
      pit_sleep_ms(TEST_FAIL_HOLD);
    } else if (get_global_state().apic_initialized >= 0xf0) {
      apic_sleep_ms(TEST_FAIL_HOLD);
    }
    sound_off();
  }
  spinlock_release(&log_lock);
#endif
}