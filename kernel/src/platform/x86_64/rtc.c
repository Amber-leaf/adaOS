#include "header/rtc.h"

#include <stdint.h>

#include "../../memory/header/heap.h"
#include "../../util/header/log.h"
#include "../../util/header/printf.h"
#include "header/acpi.h"
#include "header/apic.h"
#include "header/port.h"

#define CMOS_UPDATE_DELAY_MS 10

#define READ_BCD(reg)                                                          \
  (snprintf_(buf, sizeof(buf), "%x", read_cmos_reg(reg)), bcd_str_to_int(buf))

extern struct fadt *fadt;

static void sel_cmos_reg(uint8_t offset) { outb(0x70, offset & ~(1 << 7)); }

static uint8_t read_cmos_reg(uint8_t offset) {
  sel_cmos_reg(offset);
  apic_sleep_ms(CMOS_UPDATE_DELAY_MS);
  return inb(0x71);
}

static void write_cmos_reg(uint8_t offset, uint8_t val) {
  sel_cmos_reg(offset);
  apic_sleep_ms(CMOS_UPDATE_DELAY_MS);
  outb(0x71, val);
}

static int bcd_str_to_int(const char *s) {
  int result = 0;
  while (*s != '\0') {
    result = result * 10 + (*s - '0');
    s++;
  }
  return result;
}

/**
 * Converts milliseconds since the Unix epoch to an ISO 8601 timestamp string.
 * Output format: "YYYY-MM-DDTHH:MM:SS.mmmZ" (requires buf of at least 25
 * bytes).
 *
 * Date algorithm adapted from:
 *   http://howardhinnant.github.io/date_algorithms.html
 */
void ms_to_iso8601(uint64_t ms_epoch, char *buf, size_t buf_size) {
  int32_t ms = (int32_t)(ms_epoch % 1000);
  int64_t secs = (int64_t)(ms_epoch / 1000);

  int32_t sec = (int32_t)(secs % 60);
  secs /= 60;
  int32_t min = (int32_t)(secs % 60);
  secs /= 60;
  int32_t hr = (int32_t)(secs % 24);
  secs /= 24;

  /* Convert days-since-epoch to a civil (Gregorian) date. */
  int64_t days = secs + 719468LL;
  uint64_t era = (uint64_t)((days >= 0 ? days : days - 146096) / 146097);
  uint32_t doe = (uint32_t)(days - (int64_t)(era * 146097));
  uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  uint32_t y = (uint32_t)(yoe + era * 400);
  uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  uint32_t mp = (5 * doy + 2) / 153;
  uint32_t day = doy - (153 * mp + 2) / 5 + 1;
  uint32_t mon = mp + (mp < 10 ? 3 : -9);
  y += (mon <= 2 ? 1 : 0);

  snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", (int)y,
           (int)mon, (int)day, (int)hr, (int)min, (int)sec, (int)ms);
}

/**
 * Returns the Julian Day Number for the given Gregorian calendar date.
 * Formula from: https://en.wikipedia.org/wiki/Julian_day
 */
static int get_jdn(int day, int month, int year) {
  return (1461 * (year + 4800 + (month - 14) / 12)) / 4 +
         (367 * (month - 2 - 12 * ((month - 14) / 12))) / 12 -
         (3 * ((year + 4900 + (month - 14) / 12) / 100)) / 4 + day - 32075;
}

static uint64_t get_unix_epoch(uint8_t seconds, uint8_t minutes, uint8_t hours,
                               uint8_t day, uint8_t month, uint16_t year) {
  uint64_t jdn_diff =
      (uint64_t)(get_jdn(day, month, year) - get_jdn(1, 1, 1970));
  return jdn_diff * (60 * 60 * 24) + (uint64_t)hours * 3600 +
         (uint64_t)minutes * 60 + seconds;
}

uint64_t get_unix_timestamp(void) {
  k_debug("get unix time");

  uint32_t century;
  if (fadt->century) {
    century = read_cmos_reg(fadt->century);
  } else {
    century = 0x20;
    k_wrn("No century register in RTC, assuming 20xx.");
  }

  if (century != 0x20) {
    k_wrn("Nonsensical century `%x`.", century);
  }

  char buf[10];

  uint8_t seconds = READ_BCD(REGISTER_SECONDS);
  uint8_t minutes = READ_BCD(REGISTER_MINUTES);
  uint8_t hours = READ_BCD(REGISTER_HOURS);
  uint8_t day = READ_BCD(REGISTER_DAY_OF_MONTH);
  uint8_t month = READ_BCD(REGISTER_MONTH);

  snprintf_(buf, sizeof(buf), "%x%x", century, read_cmos_reg(REGISTER_YEAR));
  uint16_t year = (uint16_t)bcd_str_to_int(buf);

  return get_unix_epoch(seconds, minutes, hours, day, month, year);
}

char *get_formatted_time(void) {
  char *buf = kmalloc(25);
  ms_to_iso8601(get_unix_timestamp() * 1000, buf, 25);
  return buf;
}