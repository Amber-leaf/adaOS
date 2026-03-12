#ifndef ACPI_H_
#define ACPI_H_
#include <stdbool.h>
#include <stdint.h>

struct rsdp_header {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t oem_revision;
  uint32_t rsdt_address;
} __attribute__((packed));

struct xsdp_header {
  struct rsdp_header;

  uint32_t length;
  uint64_t xsdt_address;
  uint8_t extended_checksum;
  uint8_t reserved[3];
} __attribute__((packed));

struct sdt_header {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed));

void bootstrap_acpi();
struct limine_rsdp_response *get_rsdp();

#endif