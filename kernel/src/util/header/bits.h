#ifndef BITS_H_
#define BITS_H_

#include <stdint.h>
#include <stdbool.h>

uint32_t extract_bit_range(uint32_t value, uint8_t high, uint8_t low);
bool test_bit_range(uint32_t value, uint8_t high, uint8_t low,
                    uint32_t expected);
#endif