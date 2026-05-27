bits 16

e820_buffer       equ 0x5000
e820_entry_size   equ 24
e820_max_entries  equ 128

pml4_page         equ 0x100000
pdpt_page         equ 0x101000
pd_pages          equ 0x102000
page_table_pages  equ 18

code64_sel        equ 0x08
data_sel          equ 0x10
code32_sel        equ 0x18

extern bios_main
global stage2_start
global e820_count
global tsc_200ms_delta

section .text.boot
stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    call serial_init_16
    mov si, real_mode_msg
    call bios_print
    call get_e820
    call calibrate_tsc_200ms
    call enable_a20

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp code32_sel:protected_entry

bios_print:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0e
    mov bx, 0x0007
    int 0x10
    jmp bios_print
.done:
    ret

get_e820:
    xor ebx, ebx
    xor bp, bp
    mov di, e820_buffer
.next:
    cmp bp, e820_max_entries
    jae .done
    mov dword [di + 20], 1
    mov eax, 0xe820
    mov edx, 0x534d4150
    mov ecx, e820_entry_size
    int 0x15
    jc .done
    cmp eax, 0x534d4150
    jne .done
    cmp ecx, 20
    jb .done
    add di, e820_entry_size
    inc bp
    test ebx, ebx
    jnz .next
.done:
    mov [e820_count], bp
    ret

calibrate_tsc_200ms:
    rdtsc
    mov [tsc_start_low], eax
    mov [tsc_start_high], edx
    mov ah, 0x86
    mov cx, 0x0003
    mov dx, 0x0d40
    int 0x15
    jc .fallback
    rdtsc
    sub eax, [tsc_start_low]
    sbb edx, [tsc_start_high]
    test eax, eax
    jnz .store
    test edx, edx
    jnz .store
.fallback:
    mov eax, 600000000
    xor edx, edx
.store:
    mov [tsc_200ms_delta], eax
    mov [tsc_200ms_delta + 4], edx
    ret

enable_a20:
    in al, 0x92
    or al, 0x02
    and al, 0xfe
    out 0x92, al
    ret

serial_init_16:
    mov dx, 0x3f9
    xor al, al
    out dx, al
    mov dx, 0x3fb
    mov al, 0x80
    out dx, al
    mov dx, 0x3f8
    mov al, 0x01
    out dx, al
    mov dx, 0x3f9
    xor al, al
    out dx, al
    mov dx, 0x3fb
    mov al, 0x03
    out dx, al
    mov dx, 0x3fa
    mov al, 0xc7
    out dx, al
    mov dx, 0x3fc
    mov al, 0x0b
    out dx, al
    ret

align 8
gdt:
    dq 0x0000000000000000
    dq 0x00af9a000000ffff
    dq 0x00cf92000000ffff
    dq 0x00cf9a000000ffff
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt - 1
    dd gdt

real_mode_msg:
    db "EFIRAM BIOS loader", 13, 10, 0

bits 32
protected_entry:
    mov ax, data_sel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x70000

    call setup_identity_pages

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, pml4_page
    mov cr3, eax

    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    jmp code64_sel:long_mode_entry

setup_identity_pages:
    cld
    mov edi, pml4_page
    mov ecx, (page_table_pages * 4096) / 4
    xor eax, eax
    rep stosd

    mov dword [pml4_page], pdpt_page | 0x03
    mov dword [pml4_page + 4], 0

    xor ecx, ecx
.pdpt_loop:
    mov eax, ecx
    shl eax, 12
    add eax, pd_pages
    or eax, 0x03
    mov [pdpt_page + ecx * 8], eax
    mov dword [pdpt_page + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 16
    jb .pdpt_loop

    xor ebx, ebx
    mov edi, pd_pages
.pd_loop:
    mov eax, ebx
    shl eax, 21
    or eax, 0x83
    mov [edi], eax
    mov eax, ebx
    shr eax, 11
    mov [edi + 4], eax
    add edi, 8
    inc ebx
    cmp ebx, 8192
    jb .pd_loop
    ret

bits 64
default abs
long_mode_entry:
    mov ax, data_sel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x70000
    call bios_main
.halt:
    cli
    hlt
    jmp .halt

section .data
align 8
e820_count:
    dw 0
tsc_start_low:
    dd 0
tsc_start_high:
    dd 0
tsc_200ms_delta:
    dq 0
