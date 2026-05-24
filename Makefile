CC = clang
NASM = nasm
BUILD_DIR ?= build
DIST_DIR ?= dist
SERIAL ?= 0
BIOS_STAGE2_SECTORS ?= 48

EFI_APP := $(BUILD_DIR)/BOOTX64.EFI
USB_APP := $(DIST_DIR)/EFI/BOOT/BOOTX64.EFI
BIOS_BOOT := $(BUILD_DIR)/bios-boot.bin
BIOS_STAGE2 := $(BUILD_DIR)/bios-stage2.bin
BIOS_STAGE2_PAD := $(BUILD_DIR)/bios-stage2.pad
BIOS_IMG := $(BUILD_DIR)/efiram-bios.img

CFLAGS := \
	--target=x86_64-unknown-windows \
	-O2 \
	-Wall \
	-Wextra \
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

.PHONY: all bios clean usb-tree

all: $(EFI_APP)

bios: $(BIOS_IMG)

$(EFI_APP): src/efiram.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

$(BIOS_BOOT): bios/boot.asm | $(BUILD_DIR)
	$(NASM) -DSTAGE2_SECTORS=$(BIOS_STAGE2_SECTORS) -f bin $< -o $@

$(BIOS_STAGE2): bios/stage2.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

$(BIOS_STAGE2_PAD): $(BIOS_STAGE2)
	test $$(stat -c %s $<) -le $$(( $(BIOS_STAGE2_SECTORS) * 512 ))
	cp $< $@
	truncate -s $$(( $(BIOS_STAGE2_SECTORS) * 512 )) $@

$(BIOS_IMG): $(BIOS_BOOT) $(BIOS_STAGE2_PAD)
	cat $(BIOS_BOOT) $(BIOS_STAGE2_PAD) > $@

$(BUILD_DIR):
	mkdir -p $@

usb-tree: $(USB_APP)

$(USB_APP): $(EFI_APP)
	mkdir -p $(dir $@)
	cp $< $@

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
