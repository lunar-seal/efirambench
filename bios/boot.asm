bits 16
org 0x7c00

%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 48
%endif

stage2_load_segment equ 0x0000
stage2_load_offset  equ 0x8000
stage2_lba          equ 1

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [boot_drive], dl

    mov ah, 0x41
    mov bx, 0x55aa
    int 0x13
    jc disk_error
    cmp bx, 0xaa55
    jne disk_error
    test cx, 1
    jz disk_error

    mov si, disk_packet
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error

    jmp stage2_load_segment:stage2_load_offset

disk_error:
    mov si, disk_error_msg
    call print_string
.halt:
    hlt
    jmp .halt

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0e
    mov bx, 0x0007
    int 0x10
    jmp print_string
.done:
    ret

boot_drive:
    db 0

disk_packet:
    db 0x10
    db 0
    dw STAGE2_SECTORS
    dw stage2_load_offset
    dw stage2_load_segment
    dq stage2_lba

disk_error_msg:
    db "EFIRAM BIOS loader: disk read failed", 13, 10, 0

times 446 - ($ - $$) db 0
times 510 - ($ - $$) db 0
dw 0xaa55
