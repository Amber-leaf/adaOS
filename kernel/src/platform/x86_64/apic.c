#include "header/apic.h"

#include <stdbool.h>
#include <stdint.h>

#include "../../interupt/header/apic_timer.h"
#include "../../memory/header/memmap.h"
#include "../../memory/virtual/header/vmm.h"
#include "../../util/header/bits.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"
#include "../../util/header/state.h"
#include "header/msr.h"
#include "header/pic.h"
#include "header/pit.h"

static struct local_apic_r apic;

uintptr_t apic_virt;

extern pagemap_t *kernel_pagemap;

uint32_t ticks_1ms;

bool interrupt_as_timer;

struct local_apic_r get_apic() {
  uint64_t data = read_msr(MSR_IA32_APIC_BASE);

  k_debug("apic data: %016lx", data);

  apic.bootstrap_processor =
      (data >> 8) & 1;                     // Is this the bootstrap processor?
  apic.x2_apic_enabled = (data >> 10) & 1; // x2APIC mode enabled?
  apic.apic_enabled = (data >> 11) & 1;    // APIC globally enabled?
  apic.apic_address = (void *)(data & ~0xFFFULL); // Base address

  k_debug("bootstrap: %d, x2apic: %d, apic enabled: %d, apic addr: %p",
          apic.bootstrap_processor, apic.x2_apic_enabled, apic.apic_enabled,
          apic.apic_address);

  if (!apic.apic_enabled && apic.x2_apic_enabled) {
    panic("x2APIC currently unsupported");
  } else if (!apic.apic_enabled) {
    panic("APIC not enabled");
  }

  return apic;
}

void write_lvt_entry(uintptr_t offset, uint8_t idt_index) {
  uint32_t *ptr = ((uint32_t *)(apic_virt + offset));

  ptr[0] = idt_index;
  ptr[7] = 0b000001101;
}

void write_register(uintptr_t offset, uint32_t value) {
  ((volatile uint32_t *)(apic_virt + offset))[0] = value;
}

uint32_t read_apic_register(uintptr_t offset) {
  return ((volatile uint32_t *)(apic_virt + offset))[0];
}

void write_icr(icr_t isr, uint32_t apic_id) {
  if (apic_id && isr.destination_type > 0) {
    k_wrn("APIC ID has no effect with destination types greater than 0!");
  }

  write_register(APIC_ICR_HIGH, extract_bit_range(apic_id, 27, 24));

  k_debug("a %d", sizeof(icr_t));

  // write_register(APIC_ICR_LOW, (uint32_t)&isr);
  k_debug("b");
}

void apic_bootstrap_timer() {
  write_register(APIC_TIMER_DIVIDE_CONFIG, 0x3);
  write_register(APIC_TIMER_INITIAL_COUNT, 0xFFFFFFFF);

  pit_sleep_ms(600);

  write_register(APIC_LVT_TIMER, (1 << 16));

  ticks_1ms = (0xFFFFFFFF - read_apic_register(APIC_TIMER_CURRENT_COUNT)) / 600;

  k_debug("Estimated bus frequency: %dKhz", ticks_1ms);
}

void apic_start_timer() {
  write_register(APIC_LVT_TIMER, 0xf1 | 0x20000);
  write_register(APIC_TIMER_DIVIDE_CONFIG, 0x3);
  write_register(APIC_TIMER_INITIAL_COUNT, ticks_1ms);

  interrupt_as_timer = true;

  unmask_irq(0);
}

void apic_sleep_ms(uint32_t ms) {
  interrupt_as_timer = true;
  uint64_t start_ms = get_apic_ticks();
  uint64_t end_ms = start_ms + ms;

  while (get_apic_ticks() < end_ms) {
    uint64_t current_ms = get_apic_ticks();

    if (current_ms < start_ms) {
      start_ms = current_ms;
      end_ms = start_ms + ms;
    }
    asm volatile("pause");
  }
}

void apic_interrupt_ms(uint32_t ms) {
  interrupt_as_timer = false;

  write_register(APIC_TIMER_INITIAL_COUNT, ticks_1ms * ms);

  write_register(APIC_LVT_TIMER, 0xf1);
}

// FIXME: Not working
void send_ipi(uint32_t apic_id, uint8_t isr_index) {
  k_todo("Fix send_ipi!");
  icr_t icr = {
      isr_index, 0, 0, 0, 0, 0, 1, 0, 0,
  };

  write_icr(icr, apic_id);
}

void bootstrap_apic() {
  struct local_apic_r apic = get_apic();

  k_debug("mapping bs LAPIC to v%p from p%p", apic.apic_address + HIGHER_HALF,
          apic.apic_address);

  apic_virt = (uint64_t)apic.apic_address + HIGHER_HALF;

  if (!map_page(kernel_pagemap, apic_virt, (uintptr_t)apic.apic_address,
                PTE_WRITABLE | PTE_NX | PTE_PCD)) {
    panic("Could not map APIC to virtual memory!");
  }

  k_debug("apic version: %X", read_apic_register(APIC_VERSION));

  if (read_apic_register(APIC_VERSION) < 0x10) {
    panic("82489DX (APIC versions under 0x10) are unsupported.");
  }

  write_register(APIC_SPURIOUS_INT_VECTOR, 0x1F0);

  if (read_apic_register(APIC_SPURIOUS_INT_VECTOR) != 0x1F0) {
    panic("Could not enable APIC!");
  }

  k_debug("apic id: 0x%x", read_apic_register(APIC_ID));

  apic_bootstrap_timer();

  apic_start_timer();

  write_lvt_entry(APIC_LVT_THERMAL, 0xf2);
  write_lvt_entry(APIC_LVT_ERROR, 0xf6);
  //  TODO: the rest of these

  unmask_irq(4);
}

void send_eio() { write_register(APIC_EOI, 0); }