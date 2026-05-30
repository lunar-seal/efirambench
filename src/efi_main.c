#include "platform.h"

#ifndef EFIRAM_SERIAL
#define EFIRAM_SERIAL 0
#endif

typedef uint64_t UINTN;
typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef void VOID;
typedef uint16_t CHAR16;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;

#define NULL ((void *)0)
#define EFI_SUCCESS 0
#define EFI_ERROR_MASK 0x8000000000000000ULL
#define EFI_BUFFER_TOO_SMALL (EFI_ERROR_MASK | 5)

#define EFI_PAGE_SHIFT 12
#define EFI_PAGE_SIZE (1ULL << EFI_PAGE_SHIFT)

#define EFIAPI __attribute__((ms_abi))

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
} EFI_MEMORY_TYPE;

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    uint16_t ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_STATUS(EFIAPI *Reset)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *, uint8_t);
    EFI_STATUS(EFIAPI *ReadKeyStroke)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *, EFI_INPUT_KEY *);
    VOID *WaitForKey;
};

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_STATUS(EFIAPI *Reset)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, uint8_t);
    EFI_STATUS(EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, CHAR16 *);
    VOID *TestString;
    VOID *QueryMode;
    VOID *SetMode;
    VOID *SetAttribute;
    VOID *ClearScreen;
    VOID *SetCursorPosition;
    VOID *EnableCursor;
    VOID *Mode;
};

typedef struct {
    EFI_TABLE_HEADER Hdr;
    VOID *RaiseTPL;
    VOID *RestoreTPL;
    VOID *AllocatePages;
    VOID *FreePages;
    EFI_STATUS(EFIAPI *GetMemoryMap)(UINTN *, EFI_MEMORY_DESCRIPTOR *, UINTN *, UINTN *,
                                     uint32_t *);
    EFI_STATUS(EFIAPI *AllocatePool)(EFI_MEMORY_TYPE, UINTN, VOID **);
    EFI_STATUS(EFIAPI *FreePool)(VOID *);
    VOID *CreateEvent;
    VOID *SetTimer;
    VOID *WaitForEvent;
    VOID *SignalEvent;
    VOID *CloseEvent;
    VOID *CheckEvent;
    VOID *InstallProtocolInterface;
    VOID *ReinstallProtocolInterface;
    VOID *UninstallProtocolInterface;
    VOID *HandleProtocol;
    VOID *Reserved;
    VOID *RegisterProtocolNotify;
    VOID *LocateHandle;
    VOID *LocateDevicePath;
    VOID *InstallConfigurationTable;
    VOID *LoadImage;
    VOID *StartImage;
    VOID *Exit;
    VOID *UnloadImage;
    VOID *ExitBootServices;
    VOID *GetNextMonotonicCount;
    EFI_STATUS(EFIAPI *Stall)(UINTN);
    EFI_STATUS(EFIAPI *SetWatchdogTimer)(UINTN, uint64_t, UINTN, CHAR16 *);
    VOID *ConnectController;
    VOID *DisconnectController;
    VOID *OpenProtocol;
    VOID *CloseProtocol;
    VOID *OpenProtocolInformation;
    VOID *ProtocolsPerHandle;
    VOID *LocateHandleBuffer;
    VOID *LocateProtocol;
    VOID *InstallMultipleProtocolInterfaces;
    VOID *UninstallMultipleProtocolInterfaces;
    VOID *CalculateCrc32;
    VOID *CopyMem;
    VOID *SetMem;
    VOID *CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    uint32_t FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    VOID *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    VOID *ConfigurationTable;
} EFI_SYSTEM_TABLE;

static EFI_SYSTEM_TABLE *g_st;
static EFI_MEMORY_DESCRIPTOR *g_map;
static UINTN g_map_size;
static UINTN g_desc_size;
static uint64_t g_cps;

#if EFIRAM_SERIAL
static inline void io_out8(uint16_t port, uint8_t v) {
    __asm__ __volatile__("outb %0, %1" : : "a"(v), "Nd"(port));
}
static inline uint8_t io_in8(uint16_t port) {
    uint8_t v;
    __asm__ __volatile__("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static void serial_init(void) {
    io_out8(0x3f9, 0x00);
    io_out8(0x3fb, 0x80);
    io_out8(0x3f8, 0x01);
    io_out8(0x3f9, 0x00);
    io_out8(0x3fb, 0x03);
    io_out8(0x3fa, 0xc7);
    io_out8(0x3fc, 0x0b);
}
static void serial_put(char ch) {
    UINTN t = 100000;
    while (t-- > 0 && (io_in8(0x3fd) & 0x20) == 0) {}
    io_out8(0x3f8, (uint8_t)ch);
}
#else
static void serial_init(void) {}
static void serial_put(char ch) {
    (void)ch;
}
#endif

SYSV void plat_put_char(char ch) {
    CHAR16 out[3];
    if (ch == '\n') {
        serial_put('\r');
        serial_put('\n');
        out[0] = '\r';
        out[1] = '\n';
        out[2] = 0;
    } else {
        serial_put(ch);
        out[0] = (CHAR16)ch;
        out[1] = 0;
    }
    g_st->ConOut->OutputString(g_st->ConOut, out);
}

SYSV int plat_poll_key(bench_key *out) {
    EFI_INPUT_KEY k;
    if (!g_st->ConIn || g_st->ConIn->ReadKeyStroke(g_st->ConIn, &k) != EFI_SUCCESS) {
        out->special = BENCH_KEY_NONE;
        out->unicode = 0;
        return 0;
    }
    out->unicode = k.UnicodeChar;
    out->special = BENCH_KEY_NONE;
    switch (k.ScanCode) {
    case 0x01: out->special = BENCH_KEY_UP; break;
    case 0x02: out->special = BENCH_KEY_DOWN; break;
    case 0x03: out->special = BENCH_KEY_RIGHT; break;
    case 0x04: out->special = BENCH_KEY_LEFT; break;
    case 0x05: out->special = BENCH_KEY_HOME; break;
    case 0x06: out->special = BENCH_KEY_END; break;
    case 0x17: out->unicode = 0x1b; break;
    default: break;
    }
    return 1;
}

SYSV uint64_t plat_read_counter(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("lfence\n\trdtsc" : "=a"(lo), "=d"(hi) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

SYSV uint64_t plat_counter_per_sec(void) {
    return g_cps;
}

SYSV void plat_iter_usable_ranges(plat_range_cb cb, void *ctx) {
    for (UINTN off = 0; off + sizeof(EFI_MEMORY_DESCRIPTOR) <= g_map_size; off += g_desc_size) {
        EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)g_map + off);
        if (d->Type != EfiConventionalMemory || d->NumberOfPages == 0) continue;
        uint64_t s = d->PhysicalStart;
        uint64_t e = s + (d->NumberOfPages << EFI_PAGE_SHIFT);
        if (e < s) e = ~0ULL;
        if (cb(s, e, ctx)) return;
    }
}

static EFI_STATUS load_memory_map(void) {
    EFI_BOOT_SERVICES *bs = g_st->BootServices;
    UINTN sz = 0, key = 0, ds = 0;
    uint32_t dv = 0;
    EFI_STATUS s;
    VOID *buf = NULL;

    s = bs->GetMemoryMap(&sz, NULL, &key, &ds, &dv);
    if (s != EFI_BUFFER_TOO_SMALL) return s;
    if (ds == 0) ds = sizeof(EFI_MEMORY_DESCRIPTOR);
    sz += ds * 8;
    s = bs->AllocatePool(EfiLoaderData, sz, &buf);
    if (s & EFI_ERROR_MASK) return s;
    s = bs->GetMemoryMap(&sz, (EFI_MEMORY_DESCRIPTOR *)buf, &key, &ds, &dv);
    if (s & EFI_ERROR_MASK) {
        bs->FreePool(buf);
        return s;
    }
    g_map = (EFI_MEMORY_DESCRIPTOR *)buf;
    g_map_size = sz;
    g_desc_size = ds;
    return EFI_SUCCESS;
}

static uint64_t calibrate_cps(void) {
    const UINTN usec = 200000;
    uint64_t a = plat_read_counter();
    g_st->BootServices->Stall(usec);
    uint64_t b = plat_read_counter();
    uint64_t d = b - a;
    if (d == 0) return 1;
    return d * (1000000ULL / usec);
}

EFIAPI EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {
    (void)image_handle;
    g_st = st;
    serial_init();
    st->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

    EFI_STATUS s = load_memory_map();
    if (s & EFI_ERROR_MASK) {
        plat_put_char('!');
        return s;
    }
    g_cps = calibrate_cps();

    bench_main();

    g_st->BootServices->FreePool(g_map);
    return EFI_SUCCESS;
}
