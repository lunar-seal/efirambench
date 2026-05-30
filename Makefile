CC = clang
NASM = nasm
OBJCOPY ?= llvm-objcopy
BUILD_DIR ?= build
DIST_DIR ?= dist
SERIAL ?= 0
BIOS_STAGE2_SECTORS ?= 96

EFI_APP := $(BUILD_DIR)/BOOTX64.EFI
USB_APP := $(DIST_DIR)/EFI/BOOT/BOOTX64.EFI
BIOS_BOOT := $(BUILD_DIR)/bios-boot.bin
BIOS_ISO_BOOT := $(BUILD_DIR)/bios-isoboot.bin
BIOS_STAGE2_OBJ := $(BUILD_DIR)/bios-stage2.o
BIOS_MAIN_OBJ := $(BUILD_DIR)/bios_main.o
BIOS_BENCH_OBJ := $(BUILD_DIR)/bios-bench.o
BIOS_STAGE2_ELF := $(BUILD_DIR)/bios-stage2.elf
BIOS_STAGE2 := $(BUILD_DIR)/bios-stage2.bin
BIOS_STAGE2_PAD := $(BUILD_DIR)/bios-stage2.pad
BIOS_IMG := $(BUILD_DIR)/efiram-bios.img
BIOS_ISO_BOOT_IMG := $(BUILD_DIR)/efiram-bios-eltorito.img
BIOS_ISO := $(BUILD_DIR)/efiram-bios.iso
UEFI_ESP_IMG := $(BUILD_DIR)/efiram-uefi-esp.img
UEFI_ISO := $(BUILD_DIR)/efiram-uefi.iso
BIOS_ISO_BOOT_SECTORS := $$(($(BIOS_STAGE2_SECTORS) + 2))

WARNFLAGS := \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wshadow \
	-Wpointer-arith \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wundef \
	-Wnull-dereference \
	-Wfloat-equal \
	-Werror

CFLAGS := \
	--target=x86_64-unknown-windows \
	-O2 \
	$(WARNFLAGS) \
	-ffreestanding \
	-fno-builtin \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-fshort-wchar \
	-mno-red-zone \
	-DEFIRAM_SERIAL=$(SERIAL)

LDFLAGS := \
	-nostdlib \
	-fuse-ld=lld-link \
	-Wl,/entry:efi_main \
	-Xlinker /subsystem:efi_application,5.0 \
	-Wl,/base:0x100000 \
	-Wl,/filealign:512 \
	-Wl,/dynamicbase:no \
	-Wl,/nxcompat:no \
	-Wl,/highentropyva:no \
	-Wl,/tsaware:no \
	-Wl,/timestamp:0 \
	-Wl,/nodefaultlib

CLANG_TIDY ?= clang-tidy
CLANG_FORMAT ?= clang-format

C_SOURCES := src/bench.c src/efi_main.c src/bios_main.c
C_HEADERS := src/platform.h
TIDY_UEFI_SRCS := src/efi_main.c src/bench.c
TIDY_BIOS_SRCS := src/bios_main.c src/bench.c

# Drop -c for clang-tidy: it forwards flags to the driver but doesn't compile.
# Use = (not :=) so BIOS_CFLAGS (defined later in the file) is fully expanded.
BIOS_TIDY_FLAGS = $(filter-out -c,$(BIOS_CFLAGS))

.PHONY: all bios bios-iso clean format format-check isos lint lint-bios lint-uefi uefi-iso usb-tree

all: $(EFI_APP)

bios: $(BIOS_IMG)

bios-iso: $(BIOS_ISO)

uefi-iso: $(UEFI_ISO)

isos: bios-iso uefi-iso

EFI_SRCS := src/efi_main.c src/bench.c

$(EFI_APP): $(EFI_SRCS) src/platform.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(EFI_SRCS) $(LDFLAGS) -o $@

$(BIOS_BOOT): bios/boot.asm | $(BUILD_DIR)
	$(NASM) -DSTAGE2_SECTORS=$(BIOS_STAGE2_SECTORS) -f bin $< -o $@

$(BIOS_ISO_BOOT): bios/isoboot.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

BIOS_CFLAGS := \
	--target=x86_64-elf \
	-O2 \
	$(WARNFLAGS) \
	-ffreestanding \
	-fno-builtin \
	-fno-pic \
	-fno-stack-protector \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-mno-red-zone \
	-mno-sse \
	-mno-mmx \
	-mcmodel=large \
	-c

$(BIOS_STAGE2_OBJ): bios/stage2.asm | $(BUILD_DIR)
	$(NASM) -f elf64 $< -o $@

$(BIOS_MAIN_OBJ): src/bios_main.c src/platform.h | $(BUILD_DIR)
	$(CC) $(BIOS_CFLAGS) $< -o $@

$(BIOS_BENCH_OBJ): src/bench.c src/platform.h | $(BUILD_DIR)
	$(CC) $(BIOS_CFLAGS) $< -o $@

$(BIOS_STAGE2_ELF): $(BIOS_STAGE2_OBJ) $(BIOS_MAIN_OBJ) $(BIOS_BENCH_OBJ) bios/linker.ld | $(BUILD_DIR)
	$(CC) --target=x86_64-elf -nostdlib -static -no-pie -fno-pic -fuse-ld=lld \
		-Wl,-T,bios/linker.ld -Wl,--build-id=none -Wl,--no-pie \
		$(BIOS_STAGE2_OBJ) $(BIOS_MAIN_OBJ) $(BIOS_BENCH_OBJ) -o $@

$(BIOS_STAGE2): $(BIOS_STAGE2_ELF)
	$(OBJCOPY) -O binary $< $@

$(BIOS_STAGE2_PAD): $(BIOS_STAGE2)
	test $$(stat -c %s $<) -le $$(( $(BIOS_STAGE2_SECTORS) * 512 ))
	cp $< $@
	truncate -s $$(( $(BIOS_STAGE2_SECTORS) * 512 )) $@

$(BIOS_IMG): $(BIOS_BOOT) $(BIOS_STAGE2_PAD)
	cat $(BIOS_BOOT) $(BIOS_STAGE2_PAD) > $@

$(BIOS_ISO_BOOT_IMG): $(BIOS_ISO_BOOT) $(BIOS_STAGE2_PAD)
	cat $(BIOS_ISO_BOOT) /dev/zero | head -c 1024 > $@
	cat $(BIOS_STAGE2_PAD) >> $@

$(BIOS_ISO): $(BIOS_ISO_BOOT_IMG)
	xorriso -as mkisofs -R -J -V EFIRAM_BIOS -o $@ \
		-b $(notdir $(BIOS_ISO_BOOT_IMG)) -no-emul-boot \
		-boot-load-size $(BIOS_ISO_BOOT_SECTORS) \
		$(BIOS_ISO_BOOT_IMG)

$(UEFI_ESP_IMG): $(EFI_APP)
	rm -f $@
	truncate -s 4M $@
	mkfs.vfat -F 12 -n EFIRAM $@
	mmd -i $@ ::/EFI ::/EFI/BOOT
	mcopy -i $@ $(EFI_APP) ::/EFI/BOOT/BOOTX64.EFI

$(UEFI_ISO): $(UEFI_ESP_IMG)
	xorriso -as mkisofs -R -J -V EFIRAM_UEFI -o $@ \
		-e $(notdir $(UEFI_ESP_IMG)) -no-emul-boot \
		$(UEFI_ESP_IMG)

$(BUILD_DIR):
	mkdir -p $@

usb-tree: $(USB_APP)

$(USB_APP): $(EFI_APP)
	mkdir -p $(dir $@)
	cp $< $@

lint: lint-uefi lint-bios

lint-uefi:
	$(CLANG_TIDY) --quiet $(TIDY_UEFI_SRCS) -- $(CFLAGS)

lint-bios:
	$(CLANG_TIDY) --quiet $(TIDY_BIOS_SRCS) -- $(BIOS_TIDY_FLAGS)

format:
	$(CLANG_FORMAT) -i $(C_SOURCES) $(C_HEADERS)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(C_SOURCES) $(C_HEADERS)

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
