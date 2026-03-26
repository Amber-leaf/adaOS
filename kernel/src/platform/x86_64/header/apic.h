#ifndef APIC_H_
#define APIC_H_

#include <stdbool.h>
#include <stdint.h>

#define APIC_ID 0x020           /* APIC ID Register (Read/Write) */
#define APIC_VERSION 0x030      /* APIC Version Register (Read only) */
#define APIC_TPR 0x080          /* Task Priority Register (Read/Write) */
#define APIC_APR 0x090          /* Arbitration Priority Register (Read only) */
#define APIC_PPR 0x0A0          /* Processor Priority Register (Read only) */
#define APIC_EOI 0x0B0          /* EOI Register (Write only) */
#define APIC_RRD 0x0C0          /* Remote Read Register (Read only) */
#define APIC_LOGICAL_DEST 0x0D0 /* Logical Destination Register (Read/Write)   \
                                 */
#define APIC_DEST_FORMAT 0x0E0  /* Destination Format Register (Read/Write) */
#define APIC_SPURIOUS_INT_VECTOR                                               \
  0x0F0 /* Spurious Interrupt Vector Register (Read/Write) */
#define APIC_ISR_BASE                                                          \
  0x100 /* In-Service Register 0-7 (Read only, 0x100-0x170) */
#define APIC_TMR_BASE                                                          \
  0x180 /* Trigger Mode Register 0-7 (Read only, 0x180-0x1F0) */
#define APIC_IRR_BASE                                                          \
  0x200 /* Interrupt Request Register 0-7 (Read only, 0x200-0x270) */
#define APIC_ERROR_STATUS 0x280 /* Error Status Register (Read only) */
#define APIC_LVT_CMCI                                                          \
  0x2F0 /* LVT Corrected Machine Check Interrupt Register (Read/Write) */
#define APIC_ICR_LOW 0x300   /* Interrupt Command Register Low (Read/Write) */
#define APIC_ICR_HIGH 0x310  /* Interrupt Command Register High (Read/Write) */
#define APIC_LVT_TIMER 0x320 /* LVT Timer Register (Read/Write) */
#define APIC_LVT_THERMAL 0x330 /* LVT Thermal Sensor Register (Read/Write) */
#define APIC_LVT_PERF_MON                                                      \
  0x340 /* LVT Performance Monitoring Counters Register (Read/Write) */
#define APIC_LVT_LINT0 0x350 /* LVT LINT0 Register (Read/Write) */
#define APIC_LVT_LINT1 0x360 /* LVT LINT1 Register (Read/Write) */
#define APIC_LVT_ERROR 0x370 /* LVT Error Register (Read/Write) */
#define APIC_TIMER_INITIAL_COUNT                                               \
  0x380 /* Initial Count Register for Timer (Read/Write) */
#define APIC_TIMER_CURRENT_COUNT                                               \
  0x390 /* Current Count Register for Timer (Read only) */
#define APIC_TIMER_DIVIDE_CONFIG                                               \
  0x3E0 /* Divide Configuration Register for Timer (Read/Write) */

#define DESTINATION_TYPE_NORMAL 0
#define DESTINATION_TYPE_SELF 1
#define DESTINATION_TYPE_ALL 2
#define DESTINATION_TYPE_OTHER 3

typedef struct local_apic_r {
  bool bootstrap_processor;
  bool x2_apic_enabled;
  bool apic_enabled;
  void *apic_address;
} local_apic_r_t;

typedef struct __attribute__((packed)) lvt_entry {
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

typedef struct __attribute__((packed)) icr {
  uint8_t vector_number;
  uint8_t delivery_mode : 2;
  uint8_t destination_mode : 1;
  uint8_t delivery_status : 1;
  uint8_t reserved : 1;
  uint8_t init_level : 1; // these are weird.
  uint8_t not_init_level : 1;
  uint8_t destination_type : 2;
  uint16_t reserved_ : 11;
} icr_t;

void bootstrap_apic();
void bootstrap_apic_no_timer();


void apic_sleep_ms(uint32_t ms);

void send_eio();

void send_ipi(uint32_t apic_id, uint8_t isr_index);

struct local_apic_r get_apic();

#endif