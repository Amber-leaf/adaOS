#include "header/core.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crypt/header/random.h"
#include "data/logos.h"
#include "driver/header/rs232.h"
#include "header/limine.h"
#include "interupt/header/apic_timer.h"
#include "interupt/header/pit_handler.h"
#include "memory/header/heap.h"
#include "memory/header/memmap.h"
#include "memory/physical/header/pmm.h"
#include "memory/virtual/header/vmm.h"
#include "platform/x86_64/header/acpi.h"
#include "platform/x86_64/header/apic.h"
#include "platform/x86_64/header/cpu.h"
#include "platform/x86_64/header/cpuid.h"
#include "platform/x86_64/header/gdt.h"
#include "platform/x86_64/header/idt.h"
#include "platform/x86_64/header/ioapic.h"
#include "platform/x86_64/header/msr.h"
#include "platform/x86_64/header/pic.h"
#include "platform/x86_64/header/pit.h"
#include "platform/x86_64/header/rtc.h"
#include "scheduler/header/scheduler.h"
#include "scheduler/header/spinlock.h"
#include "util/header/log.h"
#include "util/header/panic.h"
#include "util/header/print_lowlevel.h"
#include "util/header/printf.h"
#include "util/header/state.h"

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((
    used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimize them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_date_at_boot_request
    bootime_request = {.id = LIMINE_DATE_AT_BOOT_REQUEST, .revision = 0};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used,
               section(".limine_requests_"
                       "start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used,
    section(
        ".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

#define BOOT_CHIME

#define PIT_SLEEP_TEST_MS 10
#define PIT_SLEEP_TEST_TOLERANCE 1

#define APIC_SLEEP_TEST_MS 10
#define APIC_SLEEP_TEST_TOLERANCE 1

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
  uint8_t *restrict pdest = (uint8_t *restrict)dest;
  const uint8_t *restrict psrc = (const uint8_t *restrict)src;

  for (size_t i = 0; i < n; i++) {
    pdest[i] = psrc[i];
  }

  return dest;
}

void *mempcpy(void *restrict dest, const void *restrict src, size_t n) {
  memcpy(dest, src, n);

  return (void *)dest + n;
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

// Halt and catch fire function.
void __attribute__((noreturn)) hcf(void) {
  for (;;) {
    asm("hlt");
  }
}

struct limine_framebuffer *get_framebuffer(void) {
  // Ensure we got a framebuffer.
  if (framebuffer_request.response == NULL ||
      framebuffer_request.response->framebuffer_count < 1) {
    // we can't print an error cause no fb.
    hcf();
  }

  return framebuffer_request.response->framebuffers[0];
}

int64_t get_boot_time(void) {
  if (bootime_request.response == NULL) {
    k_err("Could not get time at boot!");
    return 0;
  }

  return bootime_request.response->timestamp;
}

static char *version_string = "0.0.7";

void print_banner(void) {
  set_text_colour(0xe6a6a1);
  set_skew(1);
  k_putc(205);
  printf_("Welcome to adaOS, version %s!", version_string);
  k_putc(205);
  crlf();
  set_skew(0);

  set_text_colour(0xe0e0e0);

  char ts[25];
  ms_to_iso8601(get_boot_time() * 1000, ts, 25);

  printf_("The date is %s.\n\n", ts);
  printf_("Copyright (C) 2026 adaOS contributors.\n");

  print_random_logo();

  set_text_colour(0xffff66);

  printf_("Notice: Due to recent California and Colorado laws requiring age "
          "verification\nfor all OS's, adaOS is not licensed for"
          " use in California or Colorado.\nPlease, complain to your local "
          "representatives! (see "
          "https://www.house.gov/representatives/find-your-representative)\n");

  set_text_colour(0xffffff);

  printf_("See LICENCE in the source directory for details.\n");

  crlf();

  print_free_ram();
}

void test1(thread_t *te) {
  uint32_t i = 0;
  while (true) {
    thread_t *t = thread_self();
    k_debug("running %d at prio %d; %s", t->id, t->priority, t->name);
    if (i == 100) {
      thread_awake(te);
      i++;
    } else {
      i++;
    }
  }
}

void test2(void) {
  while (true) {
    thread_t *t = thread_self();
    k_debug("running %d at prio %d; %s", t->id, t->priority, t->name);
  }
}

void kmain_thread(void) {
  k_log("Starting main kernel thread.");

  thread_start((void *)test2, NULL, "test", FLAGS_NONE, ARGS_NONE);
  thread_start((void *)test2, NULL, "test", FLAGS_NONE, ARGS_NONE);
  thread_start((void *)test2, NULL, "test", FLAGS_NONE, ARGS_NONE);
  thread_start((void *)test2, NULL, "test", FLAGS_NONE, ARGS_NONE);
  thread_t *t =
      thread_start((void *)test2, NULL, "test2", FLAGS_NONE, ARGS_NONE);
  thread_start((void *)test1, NULL, "test1", FLAGS_NONE, ARGS(t));

  thread_sleep(t);
}

// Main boot entrypoint.
void kmain(void) {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    hcf();
  }

  calculate_screen_constants(get_framebuffer());
  set_state(FRAMEBUFFER_INITIALIZED, true);

  setup_random(get_boot_time() * 19650218UL);

  print_banner();

  if (!check_cpuid()) {
    k_err("CPUID not supported!");
  } else {
    set_state(CPUID_SUPPORTED, true);
    k_log("CPU: %s (%s).", get_cpu_name(), get_cpu_vendor());
    char *hypervisor = get_hypervisor_vendor();
    if (memcmp(hypervisor, NO_HYPERVISOR_TEXT, 12))
      k_log("Hypervisor: %s", get_hypervisor_vendor());
  }

  crlf();

  setup_gdt();
  set_state(GDT_INITIALIZED, true);
  k_ok("Setup GDT");

  setup_idt();
  set_state(IDT_INITIALIZED, true);
  k_ok("Setup IDT");

  setup_pic();
  set_state(PIC_INITIALIZED, true);
  k_ok("Setup PIC");

  setup_serial(19200);
  k_ok("Setup Serial");

  setup_pit();
  k_ok("Setup PIT as Bootstrap Timer");

  uint64_t old_time = get_pit_ticks();
  pit_sleep_ms(PIT_SLEEP_TEST_MS);
  uint64_t difference = get_pit_ticks() - old_time;

  if (difference > PIT_SLEEP_TEST_MS + PIT_SLEEP_TEST_TOLERANCE ||
      difference < PIT_SLEEP_TEST_MS - PIT_SLEEP_TEST_TOLERANCE) {
    k_test_fail("PIT sleep time was off by %dms!",
                difference - PIT_SLEEP_TEST_MS);
  } else {
    k_test_pass("PIT Sleep");
#ifdef BOOT_CHIME
    play_sound(1000);
    pit_sleep_ms(40);
    sound_off();
    set_state(PIT_INITIALIZED, true);
    set_state(PIT_TIMER_RUNNING, true);
#endif
  }

  // 0x800 | 0x100 | 0x001
  write_msr(MSR_IA32_EFER, 0x901);
  if (read_msr(MSR_IA32_EFER) != 0xd01) {
    k_test_fail("MSR_IA32_EFER Error: could not set IA_32e mode");
  } else {
    k_test_pass("MSR_IA32_EFER");
  }

  setup_pmm();
  k_ok("Setup PMM");

  void *p = pp_alloc();
  if (p != NULL && (uintptr_t)p < HIGHER_HALF) {
    k_test_pass("PMM Pointer Sanity Check");
    set_state(PMM_INITIALIZED, true);
  } else {
    k_test_fail("PMM Pointer was Bogus!");
  }

  pp_free(p);

  setup_vmm();
  set_state(VMM_INITIALIZED, true);
  k_ok("Setup VMM");

  setup_heap();
  k_ok("Setup Heap");

  p = kmalloc(PAGE_SIZE * 1.5); // Make sure we can allocate >PAGE_SIZE objects

  if (p != NULL && (uintptr_t)p > HIGHER_HALF) {
    k_test_pass("Heap Pointer Sanity Check");
    set_state(HEAP_INITIALIZED, true);
  } else {
    k_test_fail("Heap Pointer was Bogus!");
  }

  memset(p, 0x00, PAGE_SIZE * 1.5);

  kfree(p);

  bootstrap_apic(); // TODO: cleanly fail and use PIT as timer instead.

  k_ok("Bootstrap APIC Setup");

  uint64_t apic_old_time = get_apic_ticks();
  uint64_t pit_old_time = get_pit_ticks();

  apic_sleep_ms(APIC_SLEEP_TEST_MS);

  uint64_t apic_difference = get_apic_ticks() - apic_old_time;
  uint64_t pit_difference = get_pit_ticks() - pit_old_time;

  if ((apic_difference > APIC_SLEEP_TEST_MS + APIC_SLEEP_TEST_TOLERANCE ||
       apic_difference < APIC_SLEEP_TEST_MS - APIC_SLEEP_TEST_TOLERANCE) &&
      (pit_difference > APIC_SLEEP_TEST_MS + APIC_SLEEP_TEST_TOLERANCE ||
       pit_difference < APIC_SLEEP_TEST_MS - APIC_SLEEP_TEST_TOLERANCE)) {
    k_test_fail(
        "APIC sleep time was off by %dms or APIC and PIT did not agree!",
        apic_difference - APIC_SLEEP_TEST_MS);
  } else {
    k_test_pass("APIC Sleep");
    set_state(APIC_INITIALIZED, 0xf0);
  }

  bootstrap_acpi();

  k_ok("Parsed Basic ACPI Info");

  setup_io_apic();

  k_ok("Setup IO APIC(s)");

  setup_cpus();

  k_ok("Setup CPUs");

  setup_scheduler();

  k_ok("Started Scheduler");

  crlf();

  thread_start(kmain_thread, NULL, "Kernel main thread", FLAGS_NONE, ARGS_NONE);

  hcf();
}
