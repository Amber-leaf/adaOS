#include "header/acpi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../header/core.h"
#include "../../header/limine.h"
#include "../../memory/header/heap.h"
#include "../../memory/header/memmap.h"
#include "../../memory/virtual/header/vmm.h"
#include "../../util/header/log.h"
#include "../../util/header/panic.h"
#include "../../util/header/printf.h"

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST, .revision = 0};

extern pagemap_t *kernel_pagemap;

const char *MADT_TYPES_TO_STRING[] = {"Local APIC",
                                      "I/O APIC",
                                      "I/O APIC interrupt source override",
                                      "I/O APIC non-maskable interrupt source",
                                      "Local APIC non-maskable interrupt",
                                      "Local APIC address override",
                                      "Unkown/Unused",
                                      "Unkown/Unused",
                                      "Unkown/Unused",
                                      "Processor local x2APIC"};

struct fadt *fadt;

// TODO: this sucks.
struct madt_type_0 *local_apics;
uint32_t local_apic_num = 0;

struct madt_type_1 *io_apics;
uint32_t io_apic_num = 0;

struct madt_type_2 *io_apic_source_overrides;
uint32_t io_apic_source_override_num = 0;

struct madt_type_3 *io_apic_nmi_sources;
uint32_t io_apic_nmi_source_num = 0;

struct madt_type_4 *io_apic_nmis;
uint32_t io_apic_nmi_num = 0;

struct madt_type_5 *local_apic_address_overrides;
uint32_t local_apic_address_overrides_num = 0;

struct madt_type_9 *local_x2apics;
uint32_t local_x2apic_num = 0;

uintptr_t facs_ptr;

struct limine_rsdp_response *get_rsdp() {
  struct limine_rsdp_response *rsdp = rsdp_request.response;

  if (rsdp != NULL && rsdp->address) {
    return rsdp;
  }
  k_err("Failed getting RSDP! ACPI likely not supported.");
  return NULL;
}

void check_header_signature(struct sdt_header *header, char *expected) {
  if (memcmp(header->signature, expected, 4)) {
    char *buf = kmalloc(sizeof("Expected signature of '', got ''!") +
                        sizeof(header->signature) + sizeof(char[4]));
    sprintf(buf, "Expected signature of '%.4s', got '%.4s'!", header->signature,
            expected);

    panic(buf);
  }
}

void checksum_header(struct sdt_header *header) {
  uint8_t sum = 0;

  for (uint32_t i = 0; i < header->length; i++) {
    sum += ((uint8_t *)header)[i];
  }

  if (sum != 0) {
    panic("Bad checksum for table!");
  }
}

struct sdt_header *find_table(void *root_sdt, char signature[4]) {
  struct rsdt *rsdt = (struct rsdt *)root_sdt;

  uint32_t entries = (rsdt->h.length - sizeof(rsdt->h)) / sizeof(uint32_t);

  for (uint32_t i = 0; i < entries; i++) {
    uintptr_t phys = (uintptr_t)rsdt->outgoing_pointers[i];
    uintptr_t vert = phys + HIGHER_HALF;

    map_page(kernel_pagemap, vert, phys, PTE_NX | PTE_PCD);

    struct sdt_header *h = (struct sdt_header *)vert;

    if (!memcmp(h->signature, signature, 4)) {
      return h;
    }
  }

  return NULL;
}

void parse_madt(sdt_header_t *madt_sdt_header) {
  // TODO: this sucks.
  local_apics = kmalloc(madt_sdt_header->length);
  local_apic_num = 0;

  io_apics = kmalloc(madt_sdt_header->length);
  io_apic_num = 0;

  io_apic_source_overrides = kmalloc(madt_sdt_header->length);
  io_apic_source_override_num = 0;

  io_apic_nmi_sources = kmalloc(madt_sdt_header->length);
  io_apic_nmi_source_num = 0;

  io_apic_nmis = kmalloc(madt_sdt_header->length);
  io_apic_nmi_num = 0;

  local_apic_address_overrides = kmalloc(madt_sdt_header->length);
  local_apic_address_overrides_num = 0;

  local_x2apics = kmalloc(madt_sdt_header->length);
  local_x2apic_num = 0;

  void *header_ptr = ((void *)madt_sdt_header);

  k_debug("local APIC addres: %p",
          *(uint32_t *)(header_ptr + MADT_LAPIC_ADDRESS_OFFSET));

  k_debug("local APIC flags: %x",
          *(uint32_t *)(header_ptr + MADT_FLAGS_OFFSET));

  if (!*(uint32_t *)(header_ptr + MADT_FLAGS_OFFSET)) {
    panic("TODO: Handel edge case where dual 8259 legacy PICs installed for "
          "serial.");
  }

  size_t i = 0; // TODO: this should really be a per-type index, but the ACPI
                // speck mandates that they go in order so its probably fine...
  uint8_t last_type;

  for (size_t offset = MADT_RECORD_OFFSET; offset < madt_sdt_header->length;) {
    struct madt_record_header header =
        *(struct madt_record_header *)(header_ptr + offset);

    if (last_type != header.type) {
      i = 0;
    }
    last_type = header.type;

    switch (header.type) {
    case 0:
      local_apics[i] =
          *(struct madt_type_0 *)(header_ptr + offset +
                                  sizeof(struct madt_record_header));
      local_apic_num++;
      k_debug("Local APIC %d:", local_apics[i].apic_id);
      k_debug("- Processor ID: %d", local_apics[i].processor_id);
      k_debug("- Flags: 0x%x", local_apics[i].flags);

      break;
    case 1:
      io_apics[i] = *(struct madt_type_1 *)(header_ptr + offset +
                                            sizeof(struct madt_record_header));
      io_apic_num++;
      k_debug("IO APIC %d:", io_apics[i].io_apic_id);
      k_debug("- ID: %d", io_apics[i].io_apic_id);
      k_debug("- IOREGSEL: %p", io_apics[i].io_apic_address);
      k_debug("- Lowest IRQ Serviced: %d", io_apics[i].global_system_int_base);

      break;
    case 2:
      io_apic_source_overrides[i] =
          *(struct madt_type_2 *)(header_ptr + offset +
                                  sizeof(struct madt_record_header));
      io_apic_source_override_num++;
      k_debug("IO APIC Source Override %d:", i);
      k_debug("- Bus Source: %d", io_apic_source_overrides[i].bus_source);
      k_debug("- IRQ Source: %d", io_apic_source_overrides[i].irq_source);
      k_debug("- Flags: 0x%x", io_apic_source_overrides[i].flags);
      k_debug("- IRQ Destination (?): %d",
              io_apic_source_overrides[i].global_system_int);

      break;
    case 3:
      io_apic_nmi_sources[i] =
          *(struct madt_type_3 *)(header_ptr + offset +
                                  sizeof(struct madt_record_header));
      io_apic_nmi_source_num++;
      k_debug("IO APIC NMI Source %d:", i);
      k_debug("- NMI Source: %d", io_apic_nmi_sources[i].nmi_source);
      k_debug("- Flags: 0x%x", io_apic_nmi_sources[i].flags);
      k_debug("- IRQ Destination: %d",
              io_apic_nmi_sources[i].global_system_int);

      break;
    case 4:
      io_apic_nmis[i] =
          *(struct madt_type_4 *)(header_ptr + offset +
                                  sizeof(struct madt_record_header));
      io_apic_nmi_num++;
      k_debug("IO APIC NMI %d:", i);
      k_debug("- Processor ID: %d", io_apic_nmis[i].acpi_processor_id);
      k_debug("- Flags: 0x%x", io_apic_nmis[i].flags);
      k_debug("- LINT: %d", io_apic_nmis[i].lint);

      break;
    case 5:
      local_apic_address_overrides[i] =
          *(struct madt_type_5 *)(header_ptr + offset +
                                  sizeof(struct madt_record_header));
      local_apic_address_overrides_num++;
      k_debug("Local APIC Address Override %d:", i);
      k_debug(" - Real Local APIC Physical Address: %p",
              local_apic_address_overrides[i].lapic_phys);
      break;
    case 9:
      local_x2apics[i] =
          *(struct madt_type_9 *)(header_ptr + offset +
                                  sizeof(struct madt_record_header));
      local_x2apic_num++;
      k_debug("x2 APIC %d", local_x2apics[i].apic_id);
      k_debug("- Processor ID: %d", local_x2apics[i].processor_id);
      k_debug("- Flags: 0x%x", local_x2apics[i].flags);

      break;
    default:
      k_wrn("Unkown MADT type %d (%s)!", header.type,
            MADT_TYPES_TO_STRING[header.type]);
      break;
    }

    offset += header.length;
    i++;
  }

  if (local_apic_address_overrides_num > 0) {
    panic("TODO: Use LAPIC address overrides!");
  }
}

void bootstrap_acpi() {
  struct limine_rsdp_response *rsdp = get_rsdp();

  uintptr_t phys = rsdp->address;
  uintptr_t virt = phys + HIGHER_HALF;

  if (!map_page(kernel_pagemap, virt, phys, PTE_PCD | PTE_NX)) {
    k_err("Could not map RSDP!");
    return;
  }

  struct rsdp_header rsdp_h = *(struct rsdp_header *)virt;

  if (memcmp(rsdp_h.signature, "RSD PTR ", 7)) {
    char *buf =
        kmalloc(sizeof("Expected RSDP signature of 'RSD PTR ', got ''!") +
                sizeof(rsdp_h.signature));

    sprintf(buf, "Expected RSDP signature of 'RSD PTR ', got '%.8s'!",
            rsdp_h.signature);

    panic(buf);
  }

  if (rsdp_h.oem_revision != 0) {
    k_wrn("Only RSDP rev. 0 is supported & tested! Things might break.");
  }

  uint8_t *bytes = (uint8_t *)&rsdp_h;
  uint32_t sum = 0;

  for (size_t i = 0; i < sizeof(rsdp_h); i++) {
    sum += bytes[i];
  }

  if ((sum & 0xff) != 0) {
    panic("Bad RSDP header checksum!");
  }

  phys = rsdp_h.rsdt_address;
  virt = phys + HIGHER_HALF;

  if (!map_page(kernel_pagemap, virt, phys, PTE_PCD | PTE_NX)) {
    panic("Could not map RSDT!");
  }

  struct sdt_header *rsdt_header = (sdt_header_t *)(void *)(uintptr_t)virt;

  check_header_signature(rsdt_header, "RSDT");

  checksum_header(rsdt_header);

  sdt_header_t *madt_sdt_header = find_table(rsdt_header, "APIC");
  if (madt_sdt_header == NULL) {
    panic("Could not find LAPIC (MADT) table!");
  }

  checksum_header(madt_sdt_header);

  parse_madt(madt_sdt_header);

  sdt_header_t *fadt_sdt_header = find_table(rsdt_header, "FACP");
  if (fadt_sdt_header == NULL) {
    panic("Could not find FADT table!");
  }

  checksum_header(fadt_sdt_header);

  fadt = (struct fadt *)(uintptr_t)((void *)fadt_sdt_header);

  if (fadt->smi_command_port != 0) {
    k_todo("Support SMI Command / System Management Mode");
  }

  if (fadt->firmware_ctrl != 0) {
    facs_ptr = fadt->firmware_ctrl;
  } else if (fadt->x_firmware_control != 0) {
    k_wrn("Uing X_FACS, this might not work!");
    facs_ptr = fadt->x_firmware_control;
  } else {
    panic("Something went horribly wrong parsing FADT, FACS pointer was "
          "NULL!");
  }

  k_debug("firmware ctrl: %p", fadt->firmware_ctrl);
  k_debug("smi cmd port: %p", fadt->smi_command_port);

  k_debug("dsdt ptr: %p", fadt->dsdt);

  // TODO: parse DSDT using ACPICA. (or write parser i guess but that looks
  // like hell lol)
}
