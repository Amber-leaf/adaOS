#include "header/apic.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"
#include <stdint.h>

#define IA32_APIC_BASE_MSR 0x1B

uint64_t read_msr(uint32_t msr) {
  uint32_t lo, hi;
  asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
  return ((uint64_t)hi << 32) | lo;
}

struct APICData get_apic() {
  struct APICData apic;

  uint64_t data = read_msr(IA32_APIC_BASE_MSR);

  k_log("apic data: %016lx", data);

  apic.bootstrap_processor =
      (data >> 8) & 1;                     // Is this the bootstrap processor?
  apic.x2_apic_enabled = (data >> 10) & 1; // x2APIC mode enabled?
  apic.apic_enabled = (data >> 11) & 1;    // APIC globally enabled?
  apic.apic_address = data & ~0xFFFULL;    // Base address

  k_log("bootstrap: %d, x2apic: %d, apic: %d, apic addr: %016lx",
        apic.bootstrap_processor, apic.x2_apic_enabled, apic.apic_enabled,
        apic.apic_address);

  return apic;
}

void write_lvt_entry(uint64_t *ptr, uint8_t IDT_entry) {
  ptr[0] = IDT_entry;
  ptr[8] = 0b000;
  ptr[11] = 0;
  ptr[13] = 3;
  ptr[15] = 0;
  ptr[16] = 1;
}

void setup_apic() {
  struct APICData apic = get_apic();
  uint64_t *apic_ptr = &apic.apic_address;
  uint64_t *apic_spurious_vec_ptr = apic_ptr + 0xF0;
  uint64_t *apic_id_ptr = apic_ptr + 0x20;

  apic_spurious_vec_ptr[0] = 0xF0;
  apic_spurious_vec_ptr[8] = 1;

  if (!apic_spurious_vec_ptr[8]) {
    k_err("Could not enable to APIC!");
    panic();
  }

  k_log("APIC id: 0x%x", apic_id_ptr);

  uint64_t *apic_timer_lvt_ptr = apic_ptr + 0x320; // todo
  uint64_t *apic_thermal_lvt_ptr = apic_ptr + 0x330;
  uint64_t *apic_performance_counter_ptr =
      apic_ptr + 0x340; // tf is a performance counter? idk
  uint64_t *apic_lint0_lvt_ptr = apic_ptr + 0x350; // todo
  uint64_t *apic_lint1_lvt_ptr = apic_ptr + 0x360; // todo
  uint64_t *apic_error_lvt_ptr = apic_ptr + 0x370;

  write_lvt_entry(apic_thermal_lvt_ptr, 0xf2);
  write_lvt_entry(apic_error_lvt_ptr, 0xf6);
}

void send_eio() {
  struct APICData apic = get_apic();
  uint64_t *apic_ptr = &apic.apic_address;
  uint64_t *eoi_ptr = apic_ptr + 0xB0;

  eoi_ptr[0] = 0;
}