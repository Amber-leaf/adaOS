#include "core.h"

void debug(char* msg) {
    set_text_colour(0x888888);
    puts("[DEBUG] ");
    puts(msg); crlf();
}