typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef UINT64 UINTN;
typedef UINT64 EFI_STATUS;
typedef void *EFI_HANDLE;
typedef void VOID;
typedef UINT16 CHAR16;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;

#define NULL ((void *)0)

#if defined(__x86_64__)
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

#define EFI_SUCCESS 0
#define EFI_ERROR_MASK 0x8000000000000000ULL
#define EFIERR(code) (EFI_ERROR_MASK | (code))
#define EFI_BUFFER_TOO_SMALL EFIERR(5)

#define EFI_PAGE_SHIFT 12
#define EFI_PAGE_SIZE (1ULL << EFI_PAGE_SHIFT)
#define GIB (1024ULL * 1024ULL * 1024ULL)
#define CACHE_LINE_SIZE 64ULL
#define BENCH_PATTERN 0xa5a5a5a55a5a5a5aULL
#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))
#define MAX_PASSES_PER_REPORT (1ULL << 20)

#ifndef EFIRAM_SERIAL
#define EFIRAM_SERIAL 0
#endif

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
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    UINT32 Type;
    UINT32 Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_STATUS(EFIAPI *Reset)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, UINT8 ExtendedVerification);
    EFI_STATUS(EFIAPI *ReadKeyStroke)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This, EFI_INPUT_KEY *Key);
    VOID *WaitForKey;
};

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_STATUS(EFIAPI *Reset)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINT8 ExtendedVerification);
    EFI_STATUS(EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);
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
    EFI_STATUS(EFIAPI *GetMemoryMap)(UINTN *MemoryMapSize,
                                     EFI_MEMORY_DESCRIPTOR *MemoryMap,
                                     UINTN *MapKey,
                                     UINTN *DescriptorSize,
                                     UINT32 *DescriptorVersion);
    EFI_STATUS(EFIAPI *AllocatePool)(EFI_MEMORY_TYPE PoolType, UINTN Size, VOID **Buffer);
    EFI_STATUS(EFIAPI *FreePool)(VOID *Buffer);
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
    EFI_STATUS(EFIAPI *Stall)(UINTN Microseconds);
    EFI_STATUS(EFIAPI *SetWatchdogTimer)(UINTN Timeout,
                                         UINT64 WatchdogCode,
                                         UINTN DataSize,
                                         CHAR16 *WatchdogData);
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
    UINT32 FirmwareRevision;
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

typedef struct {
    EFI_MEMORY_DESCRIPTOR *Map;
    UINTN Size;
    UINTN Key;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
} MEMORY_MAP;

typedef struct {
    const char *Name;
    UINT64 Start;
    UINT64 End;
} BENCH_REGION;

typedef UINT64 (*BENCH_WRITE_FUNC)(const MEMORY_MAP *map,
                                   UINT64 range_start,
                                   UINT64 range_end,
                                   UINT64 pattern);

typedef struct {
    const char *Name;
    BENCH_WRITE_FUNC Write;
} BENCH_MODE;

static const BENCH_REGION Regions[] = {
    {"1-3 GiB", 1ULL * GIB, 3ULL * GIB},
    {"4-7 GiB", 4ULL * GIB, 7ULL * GIB},
    {"9-12 GiB", 9ULL * GIB, 12ULL * GIB},
};

static int efi_error(EFI_STATUS status) {
    return (status & EFI_ERROR_MASK) != 0;
}

#if EFIRAM_SERIAL
static inline void io_out8(UINT16 port, UINT8 value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline UINT8 io_in8(UINT16 port) {
    UINT8 value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
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

static void serial_put_char(char ch) {
    UINTN timeout = 100000;

    while (timeout-- > 0 && (io_in8(0x3fd) & 0x20) == 0) {
    }

    io_out8(0x3f8, (UINT8)ch);
}
#else
static void serial_init(void) {
}

static void serial_put_char(char ch) {
    (void)ch;
}
#endif

static void put_char(EFI_SYSTEM_TABLE *st, char ch) {
    CHAR16 out[3];

    if (ch == '\n') {
        serial_put_char('\r');
        serial_put_char('\n');
        out[0] = '\r';
        out[1] = '\n';
        out[2] = 0;
    } else {
        serial_put_char(ch);
        out[0] = (CHAR16)ch;
        out[1] = 0;
    }

    st->ConOut->OutputString(st->ConOut, out);
}

static void put_str(EFI_SYSTEM_TABLE *st, const char *s) {
    while (*s) {
        put_char(st, *s);
        s++;
    }
}

static void put_u64(EFI_SYSTEM_TABLE *st, UINT64 value) {
    char buf[21];
    UINTN pos = 0;

    if (value == 0) {
        put_char(st, '0');
        return;
    }

    while (value > 0 && pos < sizeof(buf)) {
        buf[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (pos > 0) {
        put_char(st, buf[--pos]);
    }
}

static void put_hex64(EFI_SYSTEM_TABLE *st, UINT64 value) {
    static const char Hex[] = "0123456789abcdef";

    put_str(st, "0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        put_char(st, Hex[(value >> shift) & 0xf]);
    }
}

static UINT64 align_up(UINT64 value, UINT64 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static UINT64 align_down(UINT64 value, UINT64 alignment) {
    return value & ~(alignment - 1);
}

static UINT64 min_u64(UINT64 a, UINT64 b) {
    return a < b ? a : b;
}

static UINT64 max_u64(UINT64 a, UINT64 b) {
    return a > b ? a : b;
}

static EFI_MEMORY_DESCRIPTOR *descriptor_at(const MEMORY_MAP *map, UINTN offset) {
    return (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)map->Map + offset);
}

static EFI_STATUS get_memory_map(EFI_SYSTEM_TABLE *st, MEMORY_MAP *out) {
    EFI_BOOT_SERVICES *bs = st->BootServices;
    UINTN map_size = 0;
    UINTN map_key = 0;
    UINTN desc_size = 0;
    UINT32 desc_version = 0;
    EFI_STATUS status;
    VOID *buffer = NULL;

    status = bs->GetMemoryMap(&map_size, NULL, &map_key, &desc_size, &desc_version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        return status;
    }

    if (desc_size == 0) {
        desc_size = sizeof(EFI_MEMORY_DESCRIPTOR);
    }
    map_size += desc_size * 8;

    status = bs->AllocatePool(EfiLoaderData, map_size, &buffer);
    if (efi_error(status)) {
        return status;
    }

    status = bs->GetMemoryMap(&map_size,
                              (EFI_MEMORY_DESCRIPTOR *)buffer,
                              &map_key,
                              &desc_size,
                              &desc_version);
    if (efi_error(status)) {
        bs->FreePool(buffer);
        return status;
    }

    out->Map = (EFI_MEMORY_DESCRIPTOR *)buffer;
    out->Size = map_size;
    out->Key = map_key;
    out->DescriptorSize = desc_size;
    out->DescriptorVersion = desc_version;
    return EFI_SUCCESS;
}

static int find_next_chunk(const MEMORY_MAP *map,
                           UINT64 range_start,
                           UINT64 range_end,
                           UINT64 cursor,
                           UINT64 *chunk_start,
                           UINT64 *chunk_end) {
    int found = 0;
    UINT64 best_start = range_end;
    UINT64 best_end = range_end;

    for (UINTN offset = 0; offset + sizeof(EFI_MEMORY_DESCRIPTOR) <= map->Size;
         offset += map->DescriptorSize) {
        EFI_MEMORY_DESCRIPTOR *desc = descriptor_at(map, offset);
        UINT64 desc_start;
        UINT64 desc_end;
        UINT64 bytes;
        UINT64 start;
        UINT64 end;

        if (desc->Type != EfiConventionalMemory || desc->NumberOfPages == 0) {
            continue;
        }

        desc_start = desc->PhysicalStart;
        bytes = desc->NumberOfPages << EFI_PAGE_SHIFT;
        desc_end = desc_start + bytes;
        if (desc_end < desc_start) {
            desc_end = ~0ULL;
        }

        start = max_u64(desc_start, range_start);
        end = min_u64(desc_end, range_end);
        if (end <= cursor || end <= start) {
            continue;
        }
        if (start < cursor) {
            start = cursor;
        }

        start = align_up(start, sizeof(UINT64));
        end = align_down(end, sizeof(UINT64));
        if (end <= start) {
            continue;
        }

        if (!found || start < best_start) {
            found = 1;
            best_start = start;
            best_end = end;
        }
    }

    if (!found) {
        return 0;
    }

    *chunk_start = best_start;
    *chunk_end = best_end;
    return 1;
}

static UINT64 conventional_bytes_in_range(const MEMORY_MAP *map, UINT64 start, UINT64 end) {
    UINT64 total = 0;
    UINT64 cursor = start;
    UINT64 chunk_start;
    UINT64 chunk_end;

    while (find_next_chunk(map, start, end, cursor, &chunk_start, &chunk_end)) {
        total += chunk_end - chunk_start;
        cursor = chunk_end;
    }

    return total;
}

static inline UINT64 read_tsc(void) {
    UINT32 lo;
    UINT32 hi;

    __asm__ __volatile__("lfence\n\t"
                         "rdtsc"
                         : "=a"(lo), "=d"(hi)
                         :
                         : "memory");
    return ((UINT64)hi << 32) | lo;
}

static inline void write_qwords(UINT64 address, UINT64 count, UINT64 pattern) {
    __asm__ __volatile__("cld\n\t"
                         "rep stosq"
                         : "+D"(address), "+c"(count)
                         : "a"(pattern)
                         : "memory");
}

static UINT64 write_region_once(const MEMORY_MAP *map,
                                UINT64 range_start,
                                UINT64 range_end,
                                UINT64 pattern) {
    UINT64 total = 0;
    UINT64 cursor = range_start;
    UINT64 chunk_start;
    UINT64 chunk_end;

    while (find_next_chunk(map, range_start, range_end, cursor, &chunk_start, &chunk_end)) {
        UINT64 bytes = chunk_end - chunk_start;
        write_qwords(chunk_start, bytes / sizeof(UINT64), pattern);
        total += bytes;
        cursor = chunk_end;
    }

    __asm__ __volatile__("sfence" ::: "memory");
    return total;
}

static UINT64 first_skip64_write64_address(UINT64 range_start, UINT64 chunk_start) {
    UINT64 address = align_up(chunk_start, CACHE_LINE_SIZE);
    UINT64 phase = (address - range_start) & ((2ULL * CACHE_LINE_SIZE) - 1ULL);

    if (phase < CACHE_LINE_SIZE) {
        address += CACHE_LINE_SIZE - phase;
    } else if (phase > CACHE_LINE_SIZE) {
        address += (2ULL * CACHE_LINE_SIZE) - phase + CACHE_LINE_SIZE;
    }

    return address;
}

static UINT64 write_region_skip64_write64_once(const MEMORY_MAP *map,
                                               UINT64 range_start,
                                               UINT64 range_end,
                                               UINT64 pattern) {
    UINT64 total = 0;
    UINT64 cursor = range_start;
    UINT64 chunk_start;
    UINT64 chunk_end;

    while (find_next_chunk(map, range_start, range_end, cursor, &chunk_start, &chunk_end)) {
        UINT64 address = first_skip64_write64_address(range_start, chunk_start);

        while (address + CACHE_LINE_SIZE <= chunk_end) {
            write_qwords(address, CACHE_LINE_SIZE / sizeof(UINT64), pattern);
            total += CACHE_LINE_SIZE;
            address += 2ULL * CACHE_LINE_SIZE;
        }

        cursor = chunk_end;
    }

    __asm__ __volatile__("sfence" ::: "memory");
    return total;
}

static const BENCH_MODE Modes[] = {
    {"linear", write_region_once},
    {"skip64/write64", write_region_skip64_write64_once},
};

static UINT64 calibrate_cycles_per_second(EFI_SYSTEM_TABLE *st) {
    UINT64 start;
    UINT64 end;
    UINT64 delta;
    const UINTN usec = 200000;

    start = read_tsc();
    st->BootServices->Stall(usec);
    end = read_tsc();
    delta = end - start;

    if (delta == 0) {
        return 1;
    }
    return delta * (1000000ULL / usec);
}

static int read_key(EFI_SYSTEM_TABLE *st, EFI_INPUT_KEY *key) {
    if (!st->ConIn || !st->ConIn->ReadKeyStroke) {
        return 0;
    }

    return st->ConIn->ReadKeyStroke(st->ConIn, key) == EFI_SUCCESS;
}

static void print_status(EFI_SYSTEM_TABLE *st, EFI_STATUS status) {
    put_str(st, "EFI status ");
    put_hex64(st, status);
    put_char(st, '\n');
}

static void print_controls(EFI_SYSTEM_TABLE *st) {
    put_str(st, "Keys: 1=1-3 GiB, 2=4-7 GiB, 3=9-12 GiB, space/r=toggle region, ");
    put_str(st, "m=toggle mode, +/-=passes per print, q/esc=quit\n\n");
}

static void print_config(EFI_SYSTEM_TABLE *st,
                         UINTN region_index,
                         UINTN mode_index,
                         UINT64 passes_per_report) {
    put_str(st, "Active region: ");
    put_str(st, Regions[region_index].Name);
    put_str(st, ", mode: ");
    put_str(st, Modes[mode_index].Name);
    put_str(st, ", print every ");
    put_u64(st, passes_per_report);
    put_str(st, " pass");
    if (passes_per_report != 1) {
        put_char(st, 'e');
        put_char(st, 's');
    }
    put_char(st, '\n');
}

static int apply_key(EFI_SYSTEM_TABLE *st,
                     EFI_INPUT_KEY key,
                     UINTN *region_index,
                     UINTN *mode_index,
                     UINT64 *passes_per_report) {
    CHAR16 ch = key.UnicodeChar;

    if (ch == 'q' || ch == 'Q' || ch == 0x1b || key.ScanCode == 0x17) {
        return 1;
    }

    if (ch == '1') {
        *region_index = 0;
        print_config(st, *region_index, *mode_index, *passes_per_report);
    } else if (ch == '2') {
        *region_index = 1;
        print_config(st, *region_index, *mode_index, *passes_per_report);
    } else if (ch == '3') {
        *region_index = 2;
        print_config(st, *region_index, *mode_index, *passes_per_report);
    } else if (ch == ' ' || ch == 'r' || ch == 'R') {
        *region_index = (*region_index + 1) % ARRAY_LEN(Regions);
        print_config(st, *region_index, *mode_index, *passes_per_report);
    } else if (ch == 'm' || ch == 'M') {
        *mode_index = (*mode_index + 1) % ARRAY_LEN(Modes);
        print_config(st, *region_index, *mode_index, *passes_per_report);
    } else if (ch == '+' || ch == '=') {
        if (*passes_per_report < MAX_PASSES_PER_REPORT) {
            *passes_per_report *= 2;
        }
        print_config(st, *region_index, *mode_index, *passes_per_report);
    } else if (ch == '-' || ch == '_') {
        if (*passes_per_report > 1) {
            *passes_per_report /= 2;
        }
        print_config(st, *region_index, *mode_index, *passes_per_report);
    }

    return 0;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {
    MEMORY_MAP map;
    EFI_STATUS status;
    UINT64 cycles_per_second;
    UINT64 report = 1;
    UINT64 passes_per_report = 1;
    UINTN region_index = 0;
    UINTN mode_index = 0;
    int stop = 0;

    (void)image_handle;

    serial_init();
    st->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

    put_str(st, "\nEFIRAM write bandwidth test\n");
    put_str(st, "Pattern: ");
    put_hex64(st, BENCH_PATTERN);
    put_char(st, '\n');
    put_str(st, "Only EfiConventionalMemory pages are touched.\n");
    print_controls(st);

    status = get_memory_map(st, &map);
    if (efi_error(status)) {
        put_str(st, "Could not read UEFI memory map: ");
        print_status(st, status);
        return status;
    }

    put_str(st, "Memory map descriptor size: ");
    put_u64(st, map.DescriptorSize);
    put_str(st, " bytes\n");

    for (UINTN i = 0; i < ARRAY_LEN(Regions); i++) {
        UINT64 bytes = conventional_bytes_in_range(&map, Regions[i].Start, Regions[i].End);
        put_str(st, Regions[i].Name);
        put_str(st, ": ");
        put_u64(st, bytes >> 20);
        put_str(st, " MiB conventional RAM available in requested range\n");
    }

    put_str(st, "\nCalibrating TSC...\n");
    cycles_per_second = calibrate_cycles_per_second(st);
    put_str(st, "Estimated TSC: ");
    put_u64(st, cycles_per_second / 1000000ULL);
    put_str(st, " MHz\n\n");
    print_config(st, region_index, mode_index, passes_per_report);

    while (!stop) {
        EFI_INPUT_KEY key;
        UINT64 start_tsc;
        UINT64 end_tsc;
        UINT64 cycles;
        UINT64 bytes = 0;
        UINT64 passes_done = 0;
        UINT64 mib;
        UINT64 millis;
        UINT64 mib_per_second;
        UINTN run_region_index;
        UINTN run_mode_index;
        EFI_INPUT_KEY pending_key;
        int have_pending_key = 0;

        if (read_key(st, &key) && apply_key(st, key, &region_index, &mode_index, &passes_per_report)) {
            break;
        }

        run_region_index = region_index;
        run_mode_index = mode_index;

        start_tsc = read_tsc();
        while (passes_done < passes_per_report) {
            bytes += Modes[run_mode_index].Write(&map,
                                             Regions[run_region_index].Start,
                                             Regions[run_region_index].End,
                                             BENCH_PATTERN);
            passes_done++;

            if (read_key(st, &key)) {
                pending_key = key;
                have_pending_key = 1;
                break;
            }
        }
        end_tsc = read_tsc();

        cycles = end_tsc - start_tsc;
        if (cycles == 0) {
            cycles = 1;
        }

        mib = bytes >> 20;
        millis = (cycles * 1000ULL) / cycles_per_second;
        mib_per_second = (mib * cycles_per_second) / cycles;

        put_str(st, "Report ");
        put_u64(st, report++);
        put_str(st, ": ");
        put_str(st, Regions[run_region_index].Name);
        put_str(st, ", ");
        put_str(st, Modes[run_mode_index].Name);
        put_str(st, ", ");
        put_u64(st, passes_done);
        put_str(st, " pass");
        if (passes_done != 1) {
            put_char(st, 'e');
            put_char(st, 's');
        }
        put_str(st, ", wrote ");
        put_u64(st, mib);
        put_str(st, " MiB in ");
        put_u64(st, millis);
        put_str(st, " ms, ");
        put_u64(st, mib_per_second);
        put_str(st, " MiB/s\n");

        if (have_pending_key) {
            stop = apply_key(st, pending_key, &region_index, &mode_index, &passes_per_report);
        }
    }

    put_str(st, "\nStopped.\n");
    st->BootServices->FreePool(map.Map);
    return EFI_SUCCESS;
}
