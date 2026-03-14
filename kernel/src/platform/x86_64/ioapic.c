#include "header/ioapic.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"

#include "../../memory/header/memmap.h"
#include "../../memory/virtual/header/vmm.h"

#include "header/acpi.h"

#include <stdint.h>
extern struct madt_type_1 *io_apics;
extern uint32_t io_apic_num;

extern struct madt_type_2 *io_apic_source_overrides;
extern uint32_t io_apic_source_override_num;

extern struct madt_type_3 *io_apic_nmi_sources;
extern uint32_t io_apic_nmi_source_num;

extern struct madt_type_4 *io_apic_nmis;
extern uint32_t io_apic_nmi_num;

extern pagemap_t *kernel_pagemap;

uint32_t extract_bit_range(uint32_t value, uint8_t high, uint8_t low) {
  uint8_t width = high - low + 1;
  uint32_t mask = (width == 32) ? 0xFFFFFFFF : ((1U << width) - 1);
  return (value >> low) & mask;
}

bool test_bit_range(uint32_t value, uint8_t high, uint8_t low,
                    uint32_t expected) {
  return extract_bit_range(value, high, low) == expected;
}

void write_ioapic_reg(uintptr_t base, uint8_t offset, uint32_t val) {
  *(volatile uint32_t *)(base) = offset;
  *(volatile uint32_t *)(base + 0x10) = val;
}

uint32_t read_ioapic_reg(uintptr_t base, uint32_t offset) {
  uint32_t volatile *ioapic = (uint32_t volatile *)base;
  // k_debug("offset: %d", offset);
  // k_debug("ioapic: %p", ioapic);

  ioapic[0] = (offset & 0xff);
  return ioapic[4];
}

void setup_ioapic() {
  k_debug("a");
  for (uint32_t i = 0; i < io_apic_num; i++) {
    uint8_t id = io_apics[i].io_apic_id;
    uint32_t base = io_apics[i].io_apic_address;

    if (!map_page(kernel_pagemap, base + HIGHER_HALF, base,
                  PTE_NX | PTE_PCD | PTE_WRITABLE)) {
      panic("Could not map page for IO APIC!");
    }

    k_debug("b");

    if (!test_bit_range(read_ioapic_reg(base + HIGHER_HALF, IO_APIC_ID), 27, 23,
                        id)) {
      panic("APIC ID did not match expected value!");
    }
    k_debug("id: 0x%x",
            extract_bit_range(read_ioapic_reg(base + HIGHER_HALF, IO_APIC_ID),
                              27, 24));

    k_debug("version: 0x%x",
            extract_bit_range(read_ioapic_reg(base + HIGHER_HALF, IO_APIC_VER),
                              7, 0));

    k_debug("max redirects: 0x%x",
            extract_bit_range(read_ioapic_reg(base + HIGHER_HALF, IO_APIC_VER),
                              23, 16));

    k_debug("arbitration priority: 0x%x",
            extract_bit_range(read_ioapic_reg(base + HIGHER_HALF, IO_APIC_ARB),
                              27, 24));
  }
}