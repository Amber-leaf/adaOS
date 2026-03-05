#include "header/printf.h"
#include <stdint.h>

/**
 * Converts an int64_t milliseconds-since-epoch value to an ISO 8601 / RFC 3339
 * timestamp string of the form: "YYYY-MM-DDTHH:MM:SS.mmmZ"
 *
 * @param ms_epoch  Milliseconds since Unix epoch (1970-01-01T00:00:00Z)
 * @param buf       Output buffer (must be at least 25 bytes)
 * @param buf_size  Size of the output buffer
 * @return          0 on success, -1 on error
 */
int ms_to_iso8601(int64_t ms_epoch, char *buf, size_t buf_size) {
  if (!buf || buf_size < 25)
    return -1;

  int32_t ms = (int32_t)(ms_epoch % 1000);
  int64_t secs = ms_epoch / 1000;

  /* Handle negative milliseconds (before epoch) */
  if (ms < 0) {
    ms += 1000;
    secs -= 1;
  }

  /* --- Decompose Unix timestamp into date/time fields --- */

  /* Clock time */
  int32_t sec = (int32_t)(secs % 60);
  secs /= 60;
  int32_t min = (int32_t)(secs % 60);
  secs /= 60;
  int32_t hr = (int32_t)(secs % 24);
  secs /= 24;

  /* secs is now days since 1970-01-01 */
  int64_t days = secs;

  /*
   * Civil date from days since epoch.
   * Algorithm: http://howardhinnant.github.io/date_algorithms.html
   * "days_from_civil" / "civil_from_days"  (public domain)
   */
  days += 719468LL; /* shift epoch to 0000-03-01 */
  int64_t era = (days >= 0 ? days : days - 146096) / 146097;
  int32_t doe = (int32_t)(days - era * 146097); /* [0, 146096] */
  int32_t yoe =
      (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; /* [0, 399] */
  int32_t y = (int32_t)(yoe + era * 400);
  int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100); /* [0, 365] */
  int32_t mp = (5 * doy + 2) / 153;                      /* [0, 11]  */
  int32_t day = doy - (153 * mp + 2) / 5 + 1;            /* [1, 31]  */
  int32_t mon = mp + (mp < 10 ? 3 : -9);                 /* [1, 12]  */
  y += (mon <= 2 ? 1 : 0);

  snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", (int)y,
           (int)mon, (int)day, (int)hr, (int)min, (int)sec, (int)ms);
  return 0;
}