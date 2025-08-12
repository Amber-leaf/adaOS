#include "core.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "log.h"

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// ---------- Kernel Start + Core Functions ----------

static uint32_t cursorx = 0;
static uint32_t cursory = 0;
static uint8_t font_size = 3;
static uint32_t on_colour = 0xffffff;
static uint32_t off_colour = 0x000000;

static uint32_t cursorx_max;
static uint32_t cursory_max;
static uint32_t bytes_per_line;
static uint32_t bytes_per_screen;
static uint32_t height;
static uint32_t width;
static uint32_t pitch;

extern uint64_t font[128];

struct xy {
    uint32_t x;
    uint32_t y;
};

struct limine_framebuffer* get_framebuffer(void) {
    return framebuffer_request.response->framebuffers[0];
}

void calculate_screen_constants(void) {
    struct limine_framebuffer* fb = get_framebuffer();

    width = fb->width;
    height = fb->height;

    cursorx_max = width / (8 * font_size);
    cursory_max = height / (8 * font_size);

    bytes_per_line = width * 4 * 8 * font_size;
    bytes_per_screen = width * height * 4;

    pitch = fb->pitch / 4;
}

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void advance_cursor(void) {
    if (cursorx + 1 > cursorx_max) {
        if (cursory + 1 > cursory_max) {
            scroll(1);
            cursorx = 0;
        }
        else {
            cursory ++;
            cursorx = 0;
        }
    }
    else {
        cursorx++;
    }
}

void nl_cursor() {
    if (cursory + 1 > cursory_max) {
        scroll(1);
        cursorx = 0;
    }
    else {
        cursorx = 0;
        cursory ++;
    }
}

void print_bitmap(uint64_t bitmap, uint32_t x, uint32_t y) {
    uint32_t *fb_ptr = get_framebuffer()->address;
    
    for (int i = 0; i < 8; ++i) {
        uint8_t row = (bitmap >> ((7 - i) * 8)) & 0xFF;

        for (int n = 0; n < 8; ++n) {
            uint32_t color = (row & (1 << (7 - n))) ? on_colour : off_colour;

            for (int dy = 0; dy < font_size; ++dy) {
                for (int dx = 0; dx < font_size; ++dx) {
                    uint32_t px = x + n * font_size + dx;
                    uint32_t py = y + i * font_size + dy;
                    fb_ptr[py * pitch + px] = color;
                }
            }
        }
    }
}

void set_text_colour(uint32_t colour) {
    on_colour = colour;
}

void set_text_bg_colour(uint32_t colour) {
    off_colour = colour;
}

void clear(void) {
    uint32_t *fb_ptr = get_framebuffer()->address;

    memset(fb_ptr, 0x00, bytes_per_screen);

    cursorx = cursory = 0;
}

struct xy calculate_xy(void) {
    struct xy xy_;
    xy_.x = cursorx * 8 * font_size;
    xy_.y = cursory * 8 * font_size;
    return xy_;
}

void putc(uint8_t c) {
    uint64_t char_bitmap = font[c];
    if (c > 127)
        char_bitmap = font[0]; // Missing char
    
    print_bitmap(char_bitmap, calculate_xy().x, calculate_xy().y);

    advance_cursor();
}


void puts(const char *s) {
    uint8_t i = 0;

    for(;;) {
        switch (s[i]) {
            case 0x00:
                return;
            case 0x0A:
                nl_cursor();
                __attribute__ ((fallthrough));
            case 0x0D:
                cursorx = 0;
                break;
            default:
                putc(s[i]);
        }
        i++;
    }
}

void puti(uint32_t n) {    
    uint32_t div = 1;
    uint32_t digit_count = 1;
    while ( div <= n / 10 ) {
        digit_count++;
        div *= 10;
    }
    while ( digit_count > 0 ) {
        uint32_t digit = (n / div) + 48;

        uint64_t char_bitmap = font[digit];
        if (digit > 57 || digit < 48)
            char_bitmap = font[0]; // Missing char
    
        print_bitmap(char_bitmap, calculate_xy().x, calculate_xy().y);

        advance_cursor();

        n %= div;
        div /= 10;
        digit_count--;
    }
}

void crlf(void) {
    puts("\r\n");
}

void scroll(uint8_t lines) {
    uint32_t *start = get_framebuffer()->address;

    for (uint8_t n = 0; n < lines; n++) {
        memcpy(start, start + cursorx_max, bytes_per_screen);
        memset(start, 0x00, bytes_per_line);
        start += cursorx;
    }
}

void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    calculate_screen_constants();

    #if 0
    for (int i = 0; i < 128*2; i++) {
        putc(i);
    }

    puts("\nHello, world!\ntest\ntest 2");
    crnl();
    puts("aaa");
    crnl();


    puti(framebuffer->width); puts(" "); puti(framebuffer->height); crnl();
    #endif

    //for (int i = 0; i < 4; i++) {
    //  puti(i); crlf();
    //}

    puts("cursorx_max: "); puti(cursorx_max); crlf();
    puts("cursory_max: "); puti(cursory_max); crlf();
    puts("bytes_per_line: "); puti(bytes_per_line); crlf();
    puts("bytes_per_screen: "); puti(bytes_per_screen); crlf();
    puts("height: "); puti(height); crlf();
    puts("width: "); puti(width); crlf();
    puts("pitch: "); puti(pitch); crlf();


    puts("the quick brown dog jumps over the lazy fox.");
    puts("the quick brown dog jumps over the lazy fox.");

    //scroll(1);

    hcf();
}
