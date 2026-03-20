#ifndef RTC_H_
#define RTC_H_

#include <stddef.h>
#include <stdint.h>

#define REGISTER_SECONDS 0x00
#define REGISTER_MINUTES 0x02
#define REGISTER_HOURS 0x04
#define REGISTER_WEEKDAY 0x06
#define REGISTER_DAY_OF_MONTH 0x07
#define REGISTER_MONTH 0x08
#define REGISTER_YEAR 0x09
#define REGISTER_CENTURY 0x32
#define REGISTER_STATUS_A 0x0A
#define REGISTER_STATUS_B 0x0B

char* get_formatted_time();
uint64_t get_unix_timestamp();
void ms_to_iso8601(uint64_t ms_epoch, char *buf, size_t buf_size);

#endif