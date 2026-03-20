#include "header/rtc.h"
#include <stdint.h>

#include "../../memory/header/heap.h"
#include "../../util/header/log.h"
#include "../../util/header/printf.h"

#include "header/acpi.h"
#include "header/apic.h"
#include "header/port.h"

#define CMOS_UPDATE_TIME 10 // ms

extern struct fadt *fadt;

void sel_cmos_reg(uint8_t offset) { outb(0x70, ~(1 << 7) & (offset)); }

uint8_t read_cmos_reg(uint8_t offset) {
  sel_cmos_reg(offset);

  apic_sleep_ms(CMOS_UPDATE_TIME); // wait a bit for cmos to update.

  return inb(0x71);
}

void write_cmos_reg(uint8_t offset, uint8_t val) {
  sel_cmos_reg(offset);

  apic_sleep_ms(CMOS_UPDATE_TIME); // wait a bit for cmos to update.

  outb(0x71, val);
}

int atoi_(const char *strg) {
  // Initialize res to 0
  int res = 0;
  int i = 0;

  // Iterate through the string and compute res
  while (strg[i] != '\0') {
    res = res * 10 + (strg[i] - '0');
    i++;
  }

  return res;
}

/**
 * Converts an int64_t milliseconds-since-epoch value to an ISO 8601 / RFC 3339
 * timestamp string of the form: "YYYY-MM-DDTHH:MM:SS.mmmZ"
 *
 * @param ms_epoch  Milliseconds since Unix epoch (1970-01-01T00:00:00Z)
 * @param buf       Output buffer (must be at least 25 bytes)
 * @param buf_size  Size of the output buffer
 *
 * Adapted from http://howardhinnant.github.io/date_algorithms.html.
 */
void ms_to_iso8601(uint64_t ms_epoch, char *buf, size_t buf_size) {
  uint32_t ms = (uint32_t)(ms_epoch % 1000);
  uint64_t secs = ms_epoch / 1000;

  /* Clock time */
  uint32_t sec = (uint32_t)(secs % 60);
  secs /= 60;
  uint32_t min = (uint32_t)(secs % 60);
  secs /= 60;
  uint32_t hr = (uint32_t)(secs % 24);
  secs /= 24;

  /* secs is now days since 1970-01-01 */
  uint64_t days = secs;

  /*
   * Civil date from days since epoch.
   * Algorithm: http://howardhinnant.github.io/date_algorithms.html
   */
  days += 719468LL; /* shift epoch to 0000-03-01 */
  uint64_t era = (days >= 0 ? days : days - 146096) / 146097;
  uint32_t doe = (uint32_t)(days - era * 146097); /* [0, 146096] */
  uint32_t yoe =
      (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; /* [0, 399] */
  uint32_t y = (uint32_t)(yoe + era * 400);
  uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100); /* [0, 365] */
  uint32_t mp = (5 * doy + 2) / 153;                      /* [0, 11]  */
  uint32_t day = doy - (153 * mp + 2) / 5 + 1;            /* [1, 31]  */
  uint32_t mon = mp + (mp < 10 ? 3 : -9);                 /* [1, 12]  */
  y += (mon <= 2 ? 1 : 0);

  snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", (int)y,
           (int)mon, (int)day, (int)hr, (int)min, (int)sec, (int)ms);
}

// Julian date calculation from https://en.wikipedia.org/wiki/Julian_day
static int get_jdn(int days, int months, int years) {
  return (1461 * (years + 4800 + (months - 14) / 12)) / 4 +
         (367 * (months - 2 - 12 * ((months - 14) / 12))) / 12 -
         (3 * ((years + 4900 + (months - 14) / 12) / 100)) / 4 + days - 32075;
}

static uint64_t get_unix_epoch(uint8_t seconds, uint8_t minutes, uint8_t hours,
                               uint8_t days, uint8_t months, uint16_t years) {
  uint64_t jdn_current = get_jdn(days, months, years);
  uint64_t jdn_1970 = get_jdn(1, 1, 1970);

  uint64_t jdn_diff = jdn_current - jdn_1970;

  return (jdn_diff * (60 * 60 * 24)) + hours * 3600 + minutes * 60 + seconds;
}

uint64_t get_unix_timestamp() {
  k_debug("get unix time");
  uint32_t century;
  if (fadt->century) {
    k_debug("read century a");

    century = read_cmos_reg(fadt->century);
    k_debug("read century");
  } else {
    century = 0x20;
    k_wrn("No century supported in RTC, assuming it's still 20xx.");
  }
  k_debug("done century");

  char seconds_buf[9];
  snprintf_(seconds_buf, 10, "%x", read_cmos_reg(REGISTER_SECONDS));

  char minutes_buf[9];
  snprintf_(minutes_buf, 10, "%x", read_cmos_reg(REGISTER_MINUTES));

  char hours_buf[9];
  snprintf_(hours_buf, 10, "%x", read_cmos_reg(REGISTER_HOURS));

  char day_buf[9];
  snprintf_(day_buf, 10, "%x", read_cmos_reg(REGISTER_DAY_OF_MONTH));

  char month_buf[9];
  snprintf_(month_buf, 10, "%x", read_cmos_reg(REGISTER_MONTH));

  char year_buf[9];
  snprintf_(year_buf, 10, "%x%x", century, read_cmos_reg(REGISTER_YEAR));

  k_debug("done unix time");

  return get_unix_epoch(atoi_(seconds_buf), atoi_(minutes_buf),
                        atoi_(hours_buf), atoi_(day_buf), atoi_(month_buf),
                        atoi_(year_buf));
}

char *get_formatted_time() {
  k_debug("Geting time");
  char *buf = kmalloc(25);
  k_debug("malloc");

  ms_to_iso8601(get_unix_timestamp() * 1000, buf, 25);

  k_debug("convert");

  return buf;
}
