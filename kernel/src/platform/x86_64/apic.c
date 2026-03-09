#include "header/apic.h"
#include "../../memory/header/memmap.h"
#include "../../memory/virtual/header/vmm.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"

#include "header/msr.h"
#include <stdint.h>

#define IA32_APIC_BASE_MSR 0x1B

static struct local_apic_r apic;

uintptr_t apic_virt;

struct local_apic_r get_apic() {
  uint64_t data = read_msr(IA32_APIC_BASE_MSR);

  k_debug("apic data: %016lx", data);

  apic.bootstrap_processor =
      (data >> 8) & 1;                     // Is this the bootstrap processor?
  apic.x2_apic_enabled = (data >> 10) & 1; // x2APIC mode enabled?
  apic.apic_enabled = (data >> 11) & 1;    // APIC globally enabled?
  apic.apic_address = (void *)(data & ~0xFFFULL); // Base address

  k_debug("bootstrap: %d, x2apic: %d, apic enabled: %d, apic addr: %016lx",
          apic.bootstrap_processor, apic.x2_apic_enabled, apic.apic_enabled,
          apic.apic_address);

  if (!apic.apic_enabled && apic.x2_apic_enabled) {
    panic("x2APIC currently unsupported");
  } else if (!apic.apic_enabled) {
    panic("APIC not enabled");
  }

  return apic;
}

void write_lvt_entry(uint32_t *ptr, uint8_t idt_index) {
  ptr[0] = idt_index;
  ptr[8] = 0b000;
  ptr[11] = 0;
  ptr[13] = 3;
  ptr[15] = 0;
  ptr[16] = 1;
}

void bootstrap_apic() {
  struct local_apic_r apic = get_apic();

  k_debug("mapping APIC to v%p from p%p", apic.apic_address + HIGHER_HALF,
          apic.apic_address);

  apic_virt = (uint64_t)apic.apic_address + HIGHER_HALF;

  if (!map_page(apic_virt, (uintptr_t)apic.apic_address,
                PTE_WRITABLE | PTE_NX)) {
    panic("Could not map APIC to virtual memory!");
  }

  uintptr_t apic_spurious_vec_ptr = apic_virt + 0xF0;
  uintptr_t apic_id_ptr = apic_virt + 0x20;
  uintptr_t apic_ver_ptr = apic_virt + 0x30;

  k_debug("apic_spurious_vec_ptr at %p", apic_spurious_vec_ptr);
  k_debug("apic_id_ptr at %p", apic_id_ptr);
  k_debug("apic_ver_ptr at %p", apic_ver_ptr);

  k_debug("apic supposed ver: %X", ((uint32_t *)(apic_ver_ptr))[0]);

  //
  // k_debug("apic version: %x", apic_ver_ptr);
  //
  // if (apic_ver_ptr < (uint32_t *)0x10) {
  //  panic("82489DX is unsupported");
  //}
  //
  // apic_spurious_vec_ptr[0] = 0xF0;
  // apic_spurious_vec_ptr[8] = 1;
  //
  // if (!apic_spurious_vec_ptr[8]) {
  //  panic("Could not enable to APIC!");
  //}
  //
  // k_debug("APIC id: 0x%x", *apic_id_ptr);
  //
  // uint32_t *apic_timer_lvt_ptr = apic_ptr + 0x320; // todo
  // uint32_t *apic_thermal_lvt_ptr = apic_ptr + 0x330;
  // uint32_t *apic_performance_counter_ptr =
  //    apic_ptr + 0x340; // tf is a performance counter? idk
  // uint32_t *apic_lint0_lvt_ptr = apic_ptr + 0x350; // todo
  // uint32_t *apic_lint1_lvt_ptr = apic_ptr + 0x360; // todo
  // uint32_t *apic_error_lvt_ptr = apic_ptr + 0x370;
  //
  // write_lvt_entry(apic_thermal_lvt_ptr, 0xf2);
  // write_lvt_entry(apic_error_lvt_ptr, 0xf6);
}

void send_eio() {
  uintptr_t eoi_ptr = apic_virt + 0xB0;

  ((uint32_t *)(eoi_ptr))[0] = 0;
}