#include "header/ioapic.h"
#include "../../util/header/bits.h"
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

extern struct madt_type_4 *lapic_nmis;
extern uint32_t lapic_nmi_num;

extern pagemap_t *kernel_pagemap;

uint8_t io_apic_system_int_base;
uint8_t io_apic_max_entries;

void write_io_apic_reg(uintptr_t base, uint8_t offset, uint32_t val) {
  uint32_t volatile *io_apic = (uint32_t volatile *)base;

  io_apic[0] = (offset & 0xff);
  io_apic[4] = val;
}

uint32_t read_io_apic_reg(uintptr_t base, uint32_t offset) {
  uint32_t volatile *io_apic = (uint32_t volatile *)base;

  io_apic[0] = (offset & 0xff);
  return io_apic[4];
}

void write_io_apic_redirection_entry(uintptr_t base,
                                     io_apic_redirection_tbl_entry_t *entry,
                                     uint8_t index) {
  if (index > io_apic_max_entries) {
    k_err("Asked to write more than max IO APIC entries!");
    return;
  } else if (index > 23) {
    k_wrn("Asked to write more than 23 entries (nominal maximum) to IO APIC!");
  }

  uint64_t bytes_of_entry = (uint64_t)entry;

  write_io_apic_reg(base, LO_REDIRECTION_FOR_N(index),
                    extract_bit_range(bytes_of_entry, 32, 0));
  write_io_apic_reg(base, HI_REDIRECTION_FOR_N(index),
                    extract_bit_range(bytes_of_entry, 64, 32));
}

void setup_io_apic() {
  // For every IO APIC...
  for (uint32_t i = 0; i < io_apic_num; i++) {
    uint8_t id = io_apics[i].io_apic_id;
    uint32_t base = io_apics[i].io_apic_address;

    uintptr_t virt_base = base + HIGHER_HALF;

    if (!map_page(kernel_pagemap, virt_base, base,
                  PTE_NX | PTE_PCD | PTE_WRITABLE)) {
      panic("Could not map page for IO APIC!");
    }

    io_apic_system_int_base = io_apics[i].global_system_int_base;
    io_apic_max_entries =
        extract_bit_range(read_io_apic_reg(virt_base, IO_APIC_VER), 23, 16);

    uint8_t highest_irq = io_apic_max_entries + io_apic_system_int_base;
    uint8_t lowest_irq = io_apic_system_int_base;

    if (!test_bit_range(read_io_apic_reg(virt_base, IO_APIC_ID), 27, 23, id)) {
      panic("APIC ID did not match expected value!");
    }

    if (io_apic_max_entries != 23) {
      k_wrn("IO_APIC with %d, not 23, IRQs serviceable; possible bug.",
            io_apic_max_entries);
    }

    k_debug("IO APIC %d:",
            extract_bit_range(read_io_apic_reg(virt_base, IO_APIC_ID), 27, 24));

    k_debug("- Version: %d",
            extract_bit_range(read_io_apic_reg(virt_base, IO_APIC_VER), 7, 0));

    k_debug("- Services IRQs: %d - %d", lowest_irq, highest_irq);

    k_debug(
        "- Arbitration priority: %d",
        extract_bit_range(read_io_apic_reg(virt_base, IO_APIC_ARB), 27, 24));

    uint8_t nmi_index = 0;

    for (uint32_t q = 0; q < io_apic_nmi_source_num; q++) {
      struct madt_type_3 nmi = io_apic_nmi_sources[q];

      if (nmi.global_system_int < highest_irq &&
          nmi.global_system_int > lowest_irq) {
        k_wrn("TODO: IO APIC NMI sources.");
      }
    }

    for (uint32_t q = 0; q < io_apic_source_override_num; q++) {
      struct madt_type_2 nmi_override = io_apic_source_overrides[q];

      if (nmi_override.global_system_int < highest_irq &&
          nmi_override.global_system_int > lowest_irq) {

        io_apic_redirection_tbl_entry_t entry;

        entry.isr_index = 0xff;

        entry.delivery_mode = 0b100;
        entry.destination_mode = 0;
        uint8_t polarity;
        uint8_t trigger_mode;
        uint8_t flags = nmi_override.flags;

        switch (extract_bit_range(flags, 1, 0)) { // Polarity switch
        case 0b11:
        case 0b00:
          polarity = 0x00;
          break;
        case 0b01:
          polarity = 0xff;
          break;
        default: // Reserved / Unkown
          k_wrn("Got Reserved / Unkown NMI Polarity.");
          polarity = 0x00;
          break;
        }

        switch (extract_bit_range(flags, 4, 2)) { // Trigger mode switch
        case 0b00:
        case 0b01:
          trigger_mode = 0x00;
          break;
        case 0b11:
          trigger_mode = 0xff;
          break;
        default: // Reserved / Unkown
          k_wrn("Got Reserved / Unkown NMI Trigger Mode.");
          trigger_mode = 0x00;
          break;
        }

        entry.polarity = polarity;
        entry.trigger_mode = trigger_mode;

        entry.delivery_mode = 0;

        write_io_apic_redirection_entry(virt_base, &entry, nmi_index);
        nmi_index++;
      }
    }
  }
}
