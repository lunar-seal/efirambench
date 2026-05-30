#ifndef PLATFORM_H
#define PLATFORM_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint64_t size_t;

#define SYSV __attribute__((sysv_abi))

#define BENCH_PATTERN 0xa5a5a5a55a5a5a5aULL
#define GIB (1024ULL * 1024ULL * 1024ULL)
#define MIB (1024ULL * 1024ULL)

#define MAX_WINDOWS 64

#define BENCH_KEY_NONE 0
#define BENCH_KEY_UP 1
#define BENCH_KEY_DOWN 2
#define BENCH_KEY_LEFT 3
#define BENCH_KEY_RIGHT 4
#define BENCH_KEY_HOME 5
#define BENCH_KEY_END 6

typedef struct {
    uint32_t special;
    uint32_t unicode;
} bench_key;

typedef struct {
    uint64_t gib_index;
    uint64_t start;
    uint64_t end;
} bench_window;

typedef int (*plat_range_cb)(uint64_t start, uint64_t end, void *ctx) SYSV;

SYSV void plat_put_char(char ch);
SYSV int plat_poll_key(bench_key *out);
SYSV uint64_t plat_read_counter(void);
SYSV uint64_t plat_counter_per_sec(void);
SYSV void plat_iter_usable_ranges(plat_range_cb cb, void *ctx);

SYSV void bench_main(void);

#endif
