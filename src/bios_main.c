#include "platform.h"

#define E820_BUFFER_ADDR 0x5000
#define E820_ENTRY_SIZE  24

extern uint16_t e820_count;
extern uint64_t tsc_200ms_delta;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attrs;
} __attribute__((packed)) e820_entry;

static volatile uint16_t *const vga = (volatile uint16_t *)0xb8000;
static uint64_t cursor_pos;

static inline void io_out8(uint16_t port, uint8_t v) {
    __asm__ __volatile__("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline uint8_t io_in8(uint16_t port) {
    uint8_t v;
    __asm__ __volatile__("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

SYSV static void serial_put(char ch) {
    for (uint64_t t = 0; t < 100000; t++) {
        if (io_in8(0x3fd) & 0x20) break;
    }
    io_out8(0x3f8, (uint8_t)ch);
}

SYSV static void scroll_if_needed(void) {
    if (cursor_pos < 80 * 25) return;
    for (uint64_t i = 0; i < 80 * 24; i++) vga[i] = vga[i + 80];
    for (uint64_t i = 80 * 24; i < 80 * 25; i++) vga[i] = 0x0720;
    cursor_pos = 80 * 24;
}

SYSV void plat_put_char(char ch) {
    if (ch == '\r') return;
    if (ch == '\n') {
        serial_put('\r');
        serial_put('\n');
        cursor_pos = ((cursor_pos / 80) + 1) * 80;
        scroll_if_needed();
        return;
    }
    serial_put(ch);
    vga[cursor_pos] = 0x0700 | (uint8_t)ch;
    cursor_pos++;
    scroll_if_needed();
}

SYSV int plat_poll_key(bench_key *out) {
    out->special = BENCH_KEY_NONE;
    out->unicode = 0;

    if ((io_in8(0x64) & 1) == 0) return 0;
    uint8_t code = io_in8(0x60);
    if (code & 0x80) return 0;

    switch (code) {
        case 0x48: out->special = BENCH_KEY_UP;   return 1;
        case 0x50: out->special = BENCH_KEY_DOWN; return 1;
        case 0x4b: out->special = BENCH_KEY_LEFT; return 1;
        case 0x4d: out->special = BENCH_KEY_RIGHT; return 1;
        case 0x47: out->special = BENCH_KEY_HOME; return 1;
        case 0x4f: out->special = BENCH_KEY_END;  return 1;
        case 0x01: out->unicode = 0x1b;   return 1;
        case 0x10: out->unicode = 'q';    return 1;
        case 0x39: out->unicode = ' ';    return 1;
        case 0x0d: out->unicode = '+';    return 1;
        case 0x4e: out->unicode = '+';    return 1;
        case 0x0c: out->unicode = '-';    return 1;
        case 0x4a: out->unicode = '-';    return 1;
        case 0x23: out->unicode = 'h';    return 1;
        case 0x13: out->unicode = 'r';    return 1;
        case 0x11: out->unicode = 'w';    return 1;
        case 0x18: out->unicode = 'o';    return 1;
        default:   return 0;
    }
}

SYSV uint64_t plat_read_counter(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

SYSV uint64_t plat_counter_per_sec(void) {
    uint64_t d = tsc_200ms_delta;
    if (d == 0) d = 600000000ULL / 5;
    return d * 5ULL;
}

SYSV void plat_iter_usable_ranges(plat_range_cb cb, void *ctx) {
    const uint8_t *p = (const uint8_t *)E820_BUFFER_ADDR;
    uint16_t n = e820_count;
    for (uint16_t i = 0; i < n; i++) {
        const e820_entry *e = (const e820_entry *)(p + i * E820_ENTRY_SIZE);
        if (e->type != 1) continue;
        if (e->length == 0) continue;
        uint64_t s = e->base;
        uint64_t end = s + e->length;
        if (end < s) end = ~0ULL;
        if (cb(s, end, ctx)) return;
    }
}

SYSV void bios_main(void) {
    cursor_pos = 0;
    for (uint64_t i = 0; i < 80 * 25; i++) vga[i] = 0x0720;
    bench_main();
}
