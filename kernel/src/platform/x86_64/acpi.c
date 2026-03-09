#include "../../header/limine.h"
#include "../../util/header/log.h"
#include <stdbool.h>

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST, .revision = 0};

struct limine_rsdp_response *get_rsdp() {
  struct limine_rsdp_response *rdsp = rsdp_request.response;

  if (rdsp->address) {
    return rdsp;
  }
  k_err("Failed getting RDSP address! ACPI likely not supported.");
  return false;
}

bool setup_acpi() {
  k_debug("rsdp addr: %p", get_rsdp());

  k_debug("TODO: ACPI using ACPICA");

  return true;
};