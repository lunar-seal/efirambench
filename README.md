# EFIRAM

Small x86_64 UEFI application that measures write bandwidth in two physical
address windows:

- `1 GiB` up to, but not including, `3 GiB`
- `4 GiB` up to, but not including, `7 GiB`
- `9 GiB` up to, but not including, `12 GiB`

For each pass it writes the fixed 64-bit pattern
`0xa5a5a5a55a5a5a5a` to ascending addresses in the active window. It starts on
`1-3 GiB` and can switch regions while running.

Two write modes are available:

- `linear`: writes every 64-bit word in the active conventional RAM chunks.
- `skip64/write64`: skips 64 bytes, writes 64 bytes, then repeats.

The app prints one report after a configurable number of passes so console
output does not dominate short runs.

The program reads the UEFI memory map and only writes pages marked
`EfiConventionalMemory`. Reserved firmware memory, MMIO holes, ACPI memory, and
the loaded EFI image are skipped.

## Runtime controls

```text
1       select 1-3 GiB
2       select 4-7 GiB
3       select 9-12 GiB
space   toggle region
r       toggle region
m       toggle write mode
+       double passes per printed report
-       halve passes per printed report
q/esc   quit
```

The output looks like:

```text
Active region: 1-3 GiB, mode: linear, print every 8 passes
Report 12: 1-3 GiB, linear, 8 passes, wrote 16384 MiB in 912 ms, 17964 MiB/s
```

## Build on NixOS

With flakes:

```sh
nix develop -c make
```

Without flakes:

```sh
nix-shell --run make
```

The EFI binary is written to:

```text
build/BOOTX64.EFI
```

To build bootable ISO images:

```sh
nix develop -c make isos
```

That creates:

```text
build/efiram-bios.iso
build/efiram-uefi.iso
```

To create the standard removable-media directory layout:

```sh
nix develop -c make usb-tree
```

or:

```sh
nix-shell --run "make usb-tree"
```

That creates:

```text
dist/EFI/BOOT/BOOTX64.EFI
```

## Put it on a USB stick

Create or reuse a FAT32 EFI System Partition on the USB stick, then copy the
binary to the removable boot path.

Example for an already formatted and mounted USB EFI partition:

```sh
sudo mkdir -p /mnt/EFI/BOOT
sudo cp build/BOOTX64.EFI /mnt/EFI/BOOT/BOOTX64.EFI
sync
sudo umount /mnt
```

If the stick is not partitioned/formatted yet, replace `/dev/sdX` with the
actual USB block device, not one of your disks:

```sh
sudo parted /dev/sdX --script mklabel gpt mkpart ESP fat32 1MiB 100% set 1 esp on
sudo mkfs.vfat -F32 /dev/sdX1
sudo mount /dev/sdX1 /mnt
sudo mkdir -p /mnt/EFI/BOOT
sudo cp build/BOOTX64.EFI /mnt/EFI/BOOT/BOOTX64.EFI
sync
sudo umount /mnt
```

Most machines require Secure Boot to be disabled unless you sign the EFI binary.

## Notes

This is a destructive RAM write test for firmware boot time. It does not run
under an operating system. The reported bandwidth is based on TSC calibrated
against UEFI `Stall()`, so treat it as a practical firmware-level estimate.
