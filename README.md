# EFIRAM

Small **Vibecoded** x86_64 UEFI and BIOS (? i don't know what the correct term is, but something that is compatible with mainboards that don't support efi) application produced with GPT 5.5.
I have not investigated the Code itself beyond skimming it. I would prefer handwritten code by someone who knows what they're doing in the future though (maybe even by myself).

This measures firmware-time memory bandwidth across fully usable physical
address windows discovered from the firmware memory map. The benchmark only
touches ranges that are entirely usable conventional RAM and skips reserved
firmware memory, MMIO holes, ACPI memory, and the loaded EFI image.

The active window size defaults to `1 GiB` and can be changed at runtime in
GiB powers of two. Candidate windows are tested on `1 GiB` boundaries, so a
`2 GiB` window can cover `4-6 GiB`, `5-7 GiB`, and so on if those spans are
fully usable.

The benchmark uses a single linear access pattern that reads or writes every
64-bit word in the active window.

The app prints one report after a configurable number of passes so console
output does not dominate short runs.

## Runtime controls

```text
up/down  previous/next usable window
home/end first/last usable window
space    next usable window
r        read mode
w        write mode
o        toggle read/write
,/.      halve/double window size
+        double passes per printed report
-        halve passes per printed report
q/esc    quit
```

The output looks like:

```text
Scanned 12 GiB of address space for fully-usable 2 GiB windows; found 5 windows
Active window: [2/4] 4-6 GiB, size: 2 GiB, mode: linear-write, print every 8 passes
Report 12: 4-6 GiB, linear-write, 8 passes, wrote 16384 MiB in 912 ms, 17964 MiB/s
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

## Firmware benchmark result

On the same mixed DDR4 host, EFIRAM reported these firmware-time write results
in `linear` mode with reports every 4 passes:

| Region | Passes | Bytes written per report | Time | Throughput |
| --- | ---: | ---: | ---: | ---: |
| 4-7 GiB | 4 | 12,288 MiB | 424 ms | ~28,970 MiB/s |
| 9-12 GiB | 4 | 12,288 MiB | 803 ms | ~15,287 MiB/s |

## Userspace comparison result

For comparison, a Linux userspace `sysbench memory` write test on a mixed DDR4
system produced the following results. The host had one 4 GiB Kingston DDR4-2133
DIMM in `DIMM_A2` and one 8 GiB Kingston DDR4-2667 DIMM in `DIMM_B2`, both
configured at 2133 MT/s.

Command:

```sh
sysbench memory \
  --memory-block-size=1G \
  --memory-total-size=100G \
  --memory-oper=write \
  --threads=1 \
  run
```

| Run | Initial `free -h` state | DMA32 free pages | Normal free pages | Throughput | Avg latency |
| --- | --- | ---: | ---: | ---: | ---: |
| 1 | 10 GiB free, 1.0 GiB used | 548,970 | 2,106,184 | 9,988.80 MiB/s | 102.49 ms |
| 2 | 6.0 GiB free, 5.2 GiB used | 548,970 | 1,014,698 | 7,349.64 MiB/s | 139.30 ms |
| 3 | 1.9 GiB free, 9.3 GiB used | 457,384 | 30,743 | 12,241.17 MiB/s | 83.63 ms |

The changing result is consistent with Linux allocation policy: normal userspace
allocations prefer the `Normal` zone over `DMA32`, so repeated runs under
increasing memory pressure can move the benchmark into different physical memory
regions. EFIRAM avoids that policy layer by writing selected physical address
windows directly at firmware boot time.

## Notes

This is a destructive RAM write test for firmware boot time. It does not run
under an operating system. The reported bandwidth is based on TSC calibrated
against UEFI `Stall()`, so treat it as a practical firmware-level estimate.
