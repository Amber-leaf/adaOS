#ifndef APIC_H_
#define APIC_H_

#include <stdint.h>
#include <stdbool.h>

struct APICData {
    bool bootstrap_processor;
    bool x2_apic_enabled; // i think this is only an itanium thing, anyway we don't support it.
    bool apic_enabled;
    uint64_t apic_address;
};

struct LVTEntry {
    uint8_t IDT_vector;
    uint8_t delivery_mode : 3; // will basically always just be 0;
    bool destination_mode : 1;
    // Delevery status, read only.
    bool delevery_status : 1;
    bool pin_polatiry : 1; // 0 is active-high, 1 is active-low.
    // Remote irr, read only;
    bool remote_IRR : 1;
    bool trigger_mode : 1; // 0 is edge-triggered, 1 is level-triggered.
    bool mask : 1; // on 1 the interrupt is disabled, if 0 is enabled.
};

struct APICData get_apic();
void setup_apic();
void send_eio();

#endif