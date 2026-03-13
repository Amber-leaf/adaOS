#include <stdint.h>
extern struct madt_type_1 *io_apics;
extern struct madt_type_2 *io_apic_source_overrides;
extern struct madt_type_3 *io_apic_nmi_sources;
extern struct madt_type_4 *io_apic_nmis;

void write_ioapic_reg(uintptr_t base, uint8_t offset, const uint32_t val) {
  *(volatile uint32_t *)(base) = offset;
  *(volatile uint32_t *)(base + 0x10) = val;
}