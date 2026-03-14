#include <stdbool.h>
#include <stdint.h>

uint32_t extract_bit_range(uint32_t value, uint8_t high, uint8_t low) {
  uint8_t width = high - low + 1;
  uint32_t mask = (width == 32) ? 0xFFFFFFFF : ((1U << width) - 1);
  return (value >> low) & mask;
}

bool test_bit_range(uint32_t value, uint8_t high, uint8_t low,
                    uint32_t expected) {
  return extract_bit_range(value, high, low) == expected;
}