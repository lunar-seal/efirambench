bits 16
org 0x7c00

stage2_load_segment equ 0x0000
stage2_load_offset  equ 0x8000

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    jmp stage2_load_segment:stage2_load_offset

times 510 - ($ - $$) db 0
dw 0xaa55
