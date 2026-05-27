#include "platform.h"

#define MAX_PASSES_PER_REPORT (1ULL << 20)

#define OP_WRITE 0
#define OP_READ  1
#define OP_COUNT 2

#define WINDOW_SIZE_MIN     GIB
#define WINDOW_SIZE_MAX     (MAX_WINDOWS * GIB)
#define WINDOW_SIZE_DEFAULT GIB

static bench_window windows[MAX_WINDOWS];
static uint64_t windows_count;
static uint64_t skipped_total;
static uint64_t skipped_leading;
static uint64_t max_gib_scanned;
static uint64_t window_size_bytes = WINDOW_SIZE_DEFAULT;
static volatile uint64_t bench_read_sink;

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t total;
} range_sum_ctx;

typedef struct {
    uint64_t pattern;
    uint64_t start;
    uint64_t end;
    uint64_t total;
} write_linear_ctx;

typedef struct {
    uint64_t scan_end;
} max_end_ctx;

SYSV static void build_windows(void);
SYSV static void print_window_summary(void);

SYSV static uint64_t align_up(uint64_t v, uint64_t a)   { return (v + a - 1) & ~(a - 1); }
SYSV static uint64_t align_down(uint64_t v, uint64_t a) { return v & ~(a - 1); }
SYSV static uint64_t min_u64(uint64_t a, uint64_t b)    { return a < b ? a : b; }
SYSV static uint64_t max_u64(uint64_t a, uint64_t b)    { return a > b ? a : b; }
SYSV static uint64_t window_size_gib(void)              { return window_size_bytes / GIB; }

SYSV static void put_str(const char *s) {
    while (*s) plat_put_char(*s++);
}

SYSV static void put_u64(uint64_t v) {
    char buf[21];
    int pos = 0;

    if (v == 0) { plat_put_char('0'); return; }
    while (v > 0) {
        buf[pos++] = (char)('0' + v % 10);
        v /= 10;
    }
    while (pos > 0) plat_put_char(buf[--pos]);
}

SYSV static void put_hex64(uint64_t v) {
    static const char hex[] = "0123456789abcdef";
    put_str("0x");
    for (int s = 60; s >= 0; s -= 4) plat_put_char(hex[(v >> s) & 0xf]);
}

SYSV static void put_window_name(uint64_t gib) {
    put_u64(gib);
    plat_put_char('-');
    put_u64(gib + window_size_gib());
    put_str(" GiB");
}

SYSV static void put_window_size(void) {
    put_u64(window_size_gib());
    put_str(" GiB");
}

SYSV static inline void write_qwords(uint64_t address, uint64_t count, uint64_t pattern) {
    __asm__ __volatile__("cld\n\t"
                         "rep stosq"
                         : "+D"(address), "+c"(count)
                         : "a"(pattern)
                         : "memory");
}

SYSV static inline uint64_t read_qwords(uint64_t address, uint64_t count) {
    const volatile uint64_t *p = (const volatile uint64_t *)address;
    uint64_t acc = 0;
    for (uint64_t i = 0; i < count; i++) {
        acc ^= p[i];
    }
    return acc;
}

SYSV static int sum_in_range_cb(uint64_t s, uint64_t e, void *ctx_) {
    range_sum_ctx *ctx = (range_sum_ctx *)ctx_;
    uint64_t a = max_u64(s, ctx->start);
    uint64_t b = min_u64(e, ctx->end);
    a = align_up(a, sizeof(uint64_t));
    b = align_down(b, sizeof(uint64_t));
    if (b > a) ctx->total += b - a;
    return 0;
}

SYSV static uint64_t usable_bytes_in_range(uint64_t start, uint64_t end) {
    range_sum_ctx ctx = { start, end, 0 };
    plat_iter_usable_ranges(sum_in_range_cb, &ctx);
    return ctx.total;
}

SYSV static int write_linear_cb(uint64_t s, uint64_t e, void *ctx_) {
    write_linear_ctx *ctx = (write_linear_ctx *)ctx_;
    uint64_t a = max_u64(s, ctx->start);
    uint64_t b = min_u64(e, ctx->end);
    a = align_up(a, sizeof(uint64_t));
    b = align_down(b, sizeof(uint64_t));
    if (b > a) {
        uint64_t bytes = b - a;
        write_qwords(a, bytes / sizeof(uint64_t), ctx->pattern);
        ctx->total += bytes;
    }
    return 0;
}

SYSV static uint64_t write_window_linear(uint64_t start, uint64_t end, uint64_t pattern) {
    write_linear_ctx ctx = { pattern, start, end, 0 };
    plat_iter_usable_ranges(write_linear_cb, &ctx);
    __asm__ __volatile__("sfence" ::: "memory");
    return ctx.total;
}

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t total;
    uint64_t acc;
} read_linear_ctx;

SYSV static int read_linear_cb(uint64_t s, uint64_t e, void *ctx_) {
    read_linear_ctx *ctx = (read_linear_ctx *)ctx_;
    uint64_t a = max_u64(s, ctx->start);
    uint64_t b = min_u64(e, ctx->end);
    a = align_up(a, sizeof(uint64_t));
    b = align_down(b, sizeof(uint64_t));
    if (b > a) {
        uint64_t bytes = b - a;
        ctx->acc ^= read_qwords(a, bytes / sizeof(uint64_t));
        ctx->total += bytes;
    }
    return 0;
}

SYSV static uint64_t read_window_linear(uint64_t start, uint64_t end, uint64_t pattern) {
    (void)pattern;
    read_linear_ctx ctx = { start, end, 0, 0 };
    plat_iter_usable_ranges(read_linear_cb, &ctx);
    bench_read_sink ^= ctx.acc;
    return ctx.total;
}
typedef SYSV uint64_t (*op_fn)(uint64_t start, uint64_t end, uint64_t pattern);

static const op_fn ops[OP_COUNT] = {
    write_window_linear,
    read_window_linear,
};

SYSV static const char *op_name(uint64_t op) {
    return (op == OP_READ) ? "read" : "write";
}

SYSV static void put_mode_label(uint64_t op) {
    put_str("linear-");
    put_str(op_name(op));
}

SYSV static int max_end_cb(uint64_t s, uint64_t e, void *ctx_) {
    (void)s;
    max_end_ctx *ctx = (max_end_ctx *)ctx_;
    if (e > ctx->scan_end) ctx->scan_end = e;
    return 0;
}

SYSV static int try_resize_windows(uint64_t new_size_bytes, uint64_t *window_index) {
    uint64_t old_size_bytes = window_size_bytes;
    uint64_t preferred_start = windows_count ? windows[*window_index].start : 0;

    window_size_bytes = new_size_bytes;
    build_windows();
    if (windows_count == 0) {
        window_size_bytes = old_size_bytes;
        build_windows();
        if (windows_count > 0 && *window_index >= windows_count) *window_index = windows_count - 1;
        put_str("Window size ");
        put_u64(new_size_bytes / GIB);
        put_str(" GiB has no fully-usable matches.\n");
        return 0;
    }

    *window_index = 0;
    while (*window_index + 1 < windows_count &&
           windows[*window_index].start < preferred_start) {
        (*window_index)++;
    }
    print_window_summary();
    return 1;
}

SYSV static void build_windows(void) {
    max_end_ctx mctx = { 0 };
    plat_iter_usable_ranges(max_end_cb, &mctx);
    uint64_t max_gib = mctx.scan_end / GIB;
    if (max_gib > MAX_WINDOWS) max_gib = MAX_WINDOWS;
    max_gib_scanned = max_gib;
    uint64_t size_gib = window_size_gib();

    windows_count = 0;
    skipped_total = 0;
    skipped_leading = 0;
    int saw_first = 0;

    if (size_gib == 0 || size_gib > max_gib) return;

    for (uint64_t g = 0; g + size_gib <= max_gib; g++) {
        uint64_t start = g * GIB;
        uint64_t end = start + window_size_bytes;
        if (usable_bytes_in_range(start, end) != window_size_bytes) {
            skipped_total++;
            if (!saw_first) skipped_leading++;
            continue;
        }
        bench_window *w = &windows[windows_count++];
        w->gib_index = g;
        w->start = start;
        w->end = end;
        saw_first = 1;
    }
}

SYSV static void print_controls(void) {
    put_str("Keys: up/down=prev/next window, home/end=first/last, space=next, ");
    put_str("r=read, w=write, o=toggle r/w, ,/.=halve/double window-size, ");
    put_str("+/-=passes per print, q/esc=quit\n\n");
}

SYSV static void print_window_summary(void) {
    put_str("Scanned ");
    put_u64(max_gib_scanned);
    put_str(" GiB of address space for fully-usable ");
    put_window_size();
    put_str(" windows; found ");
    put_u64(windows_count);
    put_str(" window");
    if (windows_count != 1) plat_put_char('s');
    put_str(" (skipped ");
    put_u64(skipped_total);
    put_str(" candidate start");
    if (skipped_total != 1) plat_put_char('s');
    put_str(", of which ");
    put_u64(skipped_leading);
    put_str(" were leading).\n");

    if (skipped_leading > 0 && windows_count > 0) {
        put_str("First matching window starts at ");
        put_window_name(windows[0].gib_index);
        put_str("; ");
        put_u64(skipped_leading);
        put_str(" earlier candidate start");
        if (skipped_leading != 1) plat_put_char('s');
        put_str(" did not have a full usable span.\n");
    }

    if (windows_count > 0) {
        put_str("Usable windows: ");
        for (uint64_t i = 0; i < windows_count; i++) {
            if (i > 0) put_str(", ");
            put_window_name(windows[i].gib_index);
        }
        plat_put_char('\n');
    }
}

SYSV static void print_config(uint64_t window_index, uint64_t op, uint64_t passes_per_report) {
    put_str("Active window: [");
    put_u64(window_index);
    plat_put_char('/');
    put_u64(windows_count ? windows_count - 1 : 0);
    put_str("] ");
    put_window_name(windows[window_index].gib_index);
    put_str(", size: ");
    put_window_size();
    put_str(", mode: ");
    put_mode_label(op);
    put_str(", print every ");
    put_u64(passes_per_report);
    put_str(" pass");
    if (passes_per_report != 1) put_str("es");
    plat_put_char('\n');
}

SYSV static int apply_key(bench_key key,
                          uint64_t *window_index,
                          uint64_t *op,
                          uint64_t *passes_per_report) {
    uint32_t ch = key.unicode;
    int changed = 0;

    if (ch == 'q' || ch == 'Q' || ch == 0x1b) return 1;

    if (key.special == BENCH_KEY_UP) {
        if (*window_index + 1 < windows_count) { (*window_index)++; changed = 1; }
    } else if (key.special == BENCH_KEY_DOWN) {
        if (*window_index > 0) { (*window_index)--; changed = 1; }
    } else if (key.special == BENCH_KEY_HOME) {
        if (windows_count > 0) { *window_index = 0; changed = 1; }
    } else if (key.special == BENCH_KEY_END) {
        if (windows_count > 0) { *window_index = windows_count - 1; changed = 1; }
    } else if (ch == ' ') {
        if (windows_count > 0) { *window_index = (*window_index + 1) % windows_count; changed = 1; }
    } else if (ch == 'r' || ch == 'R') {
        if (*op != OP_READ) { *op = OP_READ; changed = 1; }
    } else if (ch == 'w' || ch == 'W') {
        if (*op != OP_WRITE) { *op = OP_WRITE; changed = 1; }
    } else if (ch == 'o' || ch == 'O') {
        *op = (*op == OP_WRITE) ? OP_READ : OP_WRITE;
        changed = 1;
    } else if (ch == '+' || ch == '=') {
        if (*passes_per_report < MAX_PASSES_PER_REPORT) { *passes_per_report *= 2; changed = 1; }
    } else if (ch == '-' || ch == '_') {
        if (*passes_per_report > 1) { *passes_per_report /= 2; changed = 1; }
    } else if (ch == '.' || ch == '>') {
        if (window_size_bytes < WINDOW_SIZE_MAX) {
            changed = try_resize_windows(window_size_bytes * 2ULL, window_index);
        }
    } else if (ch == ',' || ch == '<') {
        if (window_size_bytes > WINDOW_SIZE_MIN) {
            changed = try_resize_windows(window_size_bytes / 2ULL, window_index);
        }
    }

    if (changed) print_config(*window_index, *op, *passes_per_report);
    return 0;
}

SYSV void bench_main(void) {
    uint64_t window_index = 0;
    uint64_t op = OP_WRITE;
    uint64_t passes_per_report = 1;
    uint64_t report = 1;
    int stop = 0;

    put_str("\nEFIRAM memory bandwidth test\n");
    put_str("Pattern: ");
    put_hex64(BENCH_PATTERN);
    plat_put_char('\n');
    put_str("Only fully-usable windows are benchmarked; size is configurable in GiB powers of two.\n");
    print_controls();

    build_windows();
    print_window_summary();

    if (windows_count == 0) {
        put_str("No fully-usable windows for the current window size; cannot benchmark.\n");
        return;
    }

    uint64_t cps = plat_counter_per_sec();
    put_str("\nEstimated TSC: ");
    put_u64(cps / 1000000ULL);
    put_str(" MHz\n\n");

    print_config(window_index, op, passes_per_report);

    while (!stop) {
        bench_key key;
        bench_key pending_key = {0, 0};
        int have_pending = 0;
        uint64_t bytes = 0;
        uint64_t passes_done = 0;

        if (plat_poll_key(&key) &&
            apply_key(key, &window_index, &op, &passes_per_report)) {
            break;
        }

        uint64_t run_window_index = window_index;
        uint64_t run_op = op;
        bench_window *w = &windows[run_window_index];

        uint64_t start_c = plat_read_counter();
        while (passes_done < passes_per_report) {
            bytes += ops[run_op](w->start, w->end, BENCH_PATTERN);
            passes_done++;
            if (plat_poll_key(&key)) {
                pending_key = key;
                have_pending = 1;
                break;
            }
        }
        uint64_t end_c = plat_read_counter();
        uint64_t cycles = end_c - start_c;
        if (cycles == 0) cycles = 1;

        uint64_t mib = bytes >> 20;
        uint64_t millis = (cycles * 1000ULL) / cps;
        uint64_t mib_per_s = (mib * cps) / cycles;

        put_str("Report ");
        put_u64(report++);
        put_str(": ");
        put_window_name(w->gib_index);
        put_str(", ");
        put_mode_label(run_op);
        put_str(", ");
        put_u64(passes_done);
        put_str(" pass");
        if (passes_done != 1) put_str("es");
        put_str(run_op == OP_READ ? ", read " : ", wrote ");
        put_u64(mib);
        put_str(" MiB in ");
        put_u64(millis);
        put_str(" ms, ");
        put_u64(mib_per_s);
        put_str(" MiB/s\n");

        if (have_pending) {
            stop = apply_key(pending_key, &window_index, &op, &passes_per_report);
        }
    }

    put_str("\nStopped.\n");
}
