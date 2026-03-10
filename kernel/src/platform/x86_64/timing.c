#include <stdbool.h>

bool _apic_enabled;

void setup_timing(bool apic_enabled) { _apic_enabled = apic_enabled; }

void sleep_ms() {}