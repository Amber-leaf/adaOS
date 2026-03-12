#ifndef ACPI_H_
#define ACPI_H_
#include <stdbool.h>
#include <stdint.h>

#define MADT_LAPIC_ADDRESS_OFFSET 0x24
#define MADT_FLAGS_OFFSET 0x28
#define MADT_RECORD_OFFSET 0x2c

#define FLAGS_POLARITY_DEFAULT 0b00
#define FLAGS_POLARITY_ACTIVE_HIGH 0b01
#define FLAGS_POLARITY_RESERVED 0b10
#define FLAGS_POLARITY_ACTIVE_LOW 0b11

#define FLAGS_TRIGGER_DEFAULT 0b00
#define FLAGS_TRIGGER_EDGE 0b01
#define FLAGS_TRIGGER_RESERVED 0b10
#define FLAGS_TRIGGER_LEVEL 0b11

typedef struct sdt_header {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed)) sdt_header_t;

struct rsdt {
  struct sdt_header h;
  uint32_t outgoing_pointers[];
};

struct rsdp_header {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t oem_revision;
  uint32_t rsdt_address;
} __attribute__((packed));

struct xsdp_header {
  struct rsdp_header rsdp_header;

  uint32_t length;
  uint64_t xsdt_address;
  uint8_t extended_checksum;
  uint8_t reserved[3];
} __attribute__((packed));

// --- MADT ---
struct madt_record_header {
  uint8_t type;
  uint8_t length;
} __attribute__((packed));

// Type 0: Processor Local APIC
struct madt_type_0 {
  uint8_t processor_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((packed));

// Type 1: I/O APIC
struct madt_type_1 {
  uint8_t io_apic_id;
  uint8_t reserved;
  uint32_t io_apic_address;
  uint32_t global_system_int_base;

} __attribute__((packed));

// Type 2: I/O APIC Interrupt Source Override
struct madt_type_2 {
  uint8_t bus_source;
  uint8_t irq_source;
  uint32_t global_system_int;
  uint16_t flags;
} __attribute__((packed));

// Type 3: I/O APIC nmi
struct madt_type_3 {
  uint8_t nmi_source;
  uint8_t reserved;
  uint16_t flags;
  uint32_t global_system_int;
} __attribute__((packed));

// Type 4: LAPIC nmi
struct madt_type_4 {
  uint8_t acpi_processor_id; // ID (0xFF = all)
  uint16_t flags;
  uint8_t lint;
} __attribute__((packed));

// Type 5: LAPIC Address Override
struct madt_type_5 {
  uint16_t reserved;
  uint64_t lapic_phys;
} __attribute__((packed));

// 6 - 8 don't exist i guess?

// Type 9: Processor Local x2APIC
struct madt_type_9 {
  uint16_t reserved;
  uint32_t processor_id;
  uint32_t flags;
  uint32_t apic_id;
} __attribute__((packed));

// --- FADT ---
typedef struct fadt_generic_address_structure {
  uint8_t address_space;
  uint8_t bit_width;
  uint8_t bit_offset;
  uint8_t access_size;
  uint64_t address;
} __attribute__((packed)) fadt_generic_address_structure_t;

struct fadt {
  struct sdt_header header;
  uint32_t firmware_ctrl;
  uint32_t dsdt;

  // field used in ACPI 1.0; no longer in use, for compatibility only
  uint8_t reserved;

  uint8_t preferred_power_management_profile;
  uint16_t sci_interrupt;
  uint32_t smi_command_port;
  uint8_t acpi_enable;
  uint8_t acpi_disable;
  uint8_t s4bios_req;
  uint8_t pstate_control;
  uint32_t pm1a_event_block;
  uint32_t pm1b_event_block;
  uint32_t pm1a_control_block;
  uint32_t pm1b_control_block;
  uint32_t pm2_control_block;
  uint32_t pm_timer_block;
  uint32_t gpe0_block;
  uint32_t gpe1_block;
  uint8_t pm1_event_length;
  uint8_t pm1_control_length;
  uint8_t pm2_control_length;
  uint8_t pm_timer_length;
  uint8_t gpe0_length;
  uint8_t gpe1_length;
  uint8_t gpe0_base;
  uint8_t c_state_control;
  uint16_t worst_c2_latency;
  uint16_t worst_c3_latency;
  uint16_t flush_size;
  uint16_t flush_stride;
  uint8_t duty_offset;
  uint8_t duty_width;
  uint8_t day_alarm;
  uint8_t month_alarm;
  uint8_t century;

  // reserved in ACPI 1.0; used since ACPI 2.0+
  uint16_t boot_architecture_flags;

  uint8_t reserved_2;
  uint32_t flags;

  // 12 byte structure; see below for details
  fadt_generic_address_structure_t reset_reg;

  uint8_t reset_value;
  uint8_t reserved_3[3];

  // 64bit pointers - Available on ACPI 2.0+
  uint64_t x_firmware_control;
  uint64_t x_dsdt;

  fadt_generic_address_structure_t x_pm1a_event_block;
  fadt_generic_address_structure_t x_pm1b_event_block;
  fadt_generic_address_structure_t x_pm1a_control_block;
  fadt_generic_address_structure_t x_pm1b_control_block;
  fadt_generic_address_structure_t x_pm2_control_block;
  fadt_generic_address_structure_t x_pm_timer_block;
  fadt_generic_address_structure_t x_gpe0_block;
  fadt_generic_address_structure_t x_gpe1_block;
} __attribute__((packed));

void bootstrap_acpi();
struct limine_rsdp_response *get_rsdp();

#endif