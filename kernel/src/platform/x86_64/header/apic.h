#ifndef APIC_H_
#define APIC_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct local_apic_r {
  bool bootstrap_processor;
  bool x2_apic_enabled;
  bool apic_enabled;
  void *apic_address;
} local_apic_r_t;

typedef struct lvt_entry {
  uint8_t IDT_vector;
  uint8_t delivery_mode : 3; // will basically always just be 0;
  bool destination_mode : 1;
  // Delevery status, read only.
  bool delevery_status : 1;
  bool pin_polatiry : 1; // 0 is active-high, 1 is active-low.
  // Remote irr, read only;
  bool remote_IRR : 1;
  bool trigger_mode : 1; // 0 is edge-triggered, 1 is level-triggered.
  bool mask : 1;         // on 1 the interrupt is disabled, if 0 is enabled.
} lvt_entry_t;


void bootstrap_apic();
void send_eio();

#endif