#ifndef IOAPIC_H_
#define IOAPIC_H_

#include <stdint.h>

#define IO_APIC_ID 0x0
#define IO_APIC_VER 0x1
#define IO_APIC_ARB 0x2
#define IO_APIC_REDIRECTION_OFFSET 0x10

#define LO_REDIRECTION_FOR_N(n) (IO_APIC_REDIRECTION_OFFSET + n * 2)
#define HI_REDIRECTION_FOR_N(n) ((IO_APIC_REDIRECTION_OFFSET + n * 2) + 1)

void setup_io_apic();

typedef struct __attribute__((packed)) io_apic_redirection_tbl_entry {
  uint8_t isr_index;
  uint8_t delivery_mode : 3; // 000 (Fixed), 001 (Lowest Priority), 010 (SMI),
                             // 100 (NMI), 101 (INIT), 111 (ExtINT)
  uint8_t destination_mode : 1; // Probably should be zero to work as expected.
  uint8_t delivery_status : 1;

  // --- Fields from the MADT ---
  uint8_t polarity : 1;
  uint8_t remote_irr : 1;
  uint8_t trigger_mode : 1;

  uint8_t mask : 1;

  uint32_t reserved;

  uint8_t phys_destination : 3;
  uint8_t log_destination : 5;
} io_apic_redirection_tbl_entry_t;

#endif