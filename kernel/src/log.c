#include "core.h"

void debug(char *msg) {
  set_text_colour(0x888888);
  k_puts("[DEBUG] ");
  k_puts(msg);
  crlf();
}