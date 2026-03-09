#include "header/core.h"
#include "header/limine.h"
#include "memory/header/heap.h"
#include "memory/header/memmap.h"
#include "memory/physical/header/pmm.h"
#include "memory/virtual/header/vmm.h"
#include "platform/x86_64/header/acpi.h"
#include "platform/x86_64/header/apic.h"
#include "platform/x86_64/header/cpuid.h"
#include "platform/x86_64/header/gdt.h"
#include "platform/x86_64/header/idt.h"
#include "platform/x86_64/header/msr.h"
#include "platform/x86_64/header/pic.h"
#include "platform/x86_64/header/pit.h"
#include "util/header/date.h"
#include "util/header/log.h"
#include "util/header/panic.h"
#include "util/header/print_lowlevel.h"
#include "util/header/printf.h"

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

static char *version_string = "0.0.3";

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
  ms_to_iso8601(get_boot_time() * 1000, ts, sizeof(ts));

  printf_("The date is %s.\n\n", ts);
  printf_("Copyright (C) 2026 Ambersoft Technologies.\n");

  set_text_colour(0xffff66);

  printf_("Notice: Due to recent California and Colorado laws requiring age "
          "verification\nfor all OS's, adaOS is not licensed for"
          " use in California or Colorado.\nPlease, complain to your local "
          "representatives! (see "
          "https://www.house.gov/representatives/find-your-representative)\n");

  set_text_colour(0xffffff);

  printf_("See LICENCE in the source directory for details.\n");

  print_free_ram();
}

// Main boot entrypoint.
void kmain(void) {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    hcf();
  }

  calculate_screen_constants(get_framebuffer());

  print_banner();

  if (!check_cpuid()) {
    panic("CPUID not Supported!");
  }
  k_log("CPU info: %s (%s).\nHypervisor: %s", get_cpu_name(), get_cpu_vendor(),
        get_hypervisor_vendor());

  crlf();

  setup_gdt();

  k_ok("GDT Init");

  setup_idt();

  k_ok("IDT Init");

  // 0x800 | 0x100 | 0x001
  write_msr(IA32_EFER, 0x901);
  if (read_msr(IA32_EFER) != 0xd01) {
    k_test_fail("IA32_EFER Error: could not set IA_32e mode");
  }
  k_test_pass("IA32_EFER"); // TODO: write more tests.

  setup_pmm();

  k_ok("Setup PMM");

  setup_vmm();

  k_ok("Setup VMM");

  setup_heap();

  k_ok("Setup Heap");

  setup_pic();

  k_ok("Setup PIC");

  setup_pit();

  k_ok("Setup PIT as Bootstrap Timer");

  hcf();

  bootstrap_apic();

  k_ok("Bootstrap APIC Setup");

  if (setup_acpi()) {
    k_ok("Setup ACPI");
  } else {
    k_err("APIC Setup Failed! Things may break!");
  }

  k_log("Halt");
  hcf();
}
