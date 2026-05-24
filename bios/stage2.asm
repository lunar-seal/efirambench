bits 16
org 0x8000

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

pattern           equ 0xa5a5a5a55a5a5a5a

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
    call main64
.halt:
    hlt
    jmp .halt

main64:
    call clear_screen
    mov rsi, header_msg
    call print_string

    mov rsi, e820_msg
    call print_string
    movzx rax, word [e820_count]
    call print_u64
    mov rsi, newline
    call print_string

    xor rdi, rdi
    call print_available_range
    mov rdi, 1
    call print_available_range
    mov rdi, 2
    call print_available_range

    mov rax, [tsc_200ms_delta]
    test rax, rax
    jnz .have_delta
    mov rax, 600000000
.have_delta:
    mov rbx, 5
    mul rbx
    mov [cycles_per_second], rax

    mov rsi, tsc_msg
    call print_string
    mov rax, [cycles_per_second]
    xor rdx, rdx
    mov rbx, 1000000
    div rbx
    call print_u64
    mov rsi, mhz_msg
    call print_string

    call print_config

.main_loop:
    call poll_key
    test al, al
    jz .start_run
    call apply_key
    test al, al
    jnz .stopped

.start_run:
    mov rax, [active_region]
    mov [run_region], rax
    mov rax, [active_mode]
    mov [run_mode], rax
    mov rax, [passes_per_report]
    mov [run_ppr], rax
    mov qword [bytes_total], 0
    mov qword [passes_done], 0
    mov byte [pending_key], 0

    call rdtsc64
    mov [tsc_start], rax

.pass_loop:
    mov rdi, [run_region]
    mov rsi, [run_mode]
    call write_region
    add [bytes_total], rax
    inc qword [passes_done]

    call poll_key
    test al, al
    jz .no_key_midrun
    mov [pending_key], al
    jmp .finish_run

.no_key_midrun:
    mov rax, [passes_done]
    cmp rax, [run_ppr]
    jb .pass_loop

.finish_run:
    call rdtsc64
    sub rax, [tsc_start]
    jnz .cycles_ok
    inc rax
.cycles_ok:
    mov [cycles_last], rax
    call print_report

    mov al, [pending_key]
    test al, al
    jz .main_loop
    call apply_key
    test al, al
    jz .main_loop

.stopped:
    mov rsi, stopped_msg
    call print_string
    ret

print_available_range:
    push rdi
    call print_region_name
    mov rsi, available_mid_msg
    call print_string
    pop rdi
    call conventional_bytes
    shr rax, 20
    call print_u64
    mov rsi, available_end_msg
    call print_string
    ret

print_config:
    mov rsi, active_region_msg
    call print_string
    mov rdi, [active_region]
    call print_region_name
    mov rsi, mode_prefix_msg
    call print_string
    mov rdi, [active_mode]
    call print_mode_name
    mov rsi, passes_prefix_msg
    call print_string
    mov rax, [passes_per_report]
    call print_u64
    mov rsi, pass_word
    call print_string
    mov rax, [passes_per_report]
    cmp rax, 1
    je .no_plural
    mov rsi, es_suffix
    call print_string
.no_plural:
    mov rsi, newline
    call print_string
    ret

print_report:
    mov rsi, report_msg
    call print_string
    mov rax, [report_no]
    call print_u64
    inc qword [report_no]
    mov rsi, colon_space
    call print_string

    mov rdi, [run_region]
    call print_region_name
    mov rsi, comma_space
    call print_string
    mov rdi, [run_mode]
    call print_mode_name
    mov rsi, comma_space
    call print_string

    mov rax, [passes_done]
    call print_u64
    mov rsi, pass_word
    call print_string
    mov rax, [passes_done]
    cmp rax, 1
    je .report_no_plural
    mov rsi, es_suffix
    call print_string
.report_no_plural:
    mov rsi, wrote_msg
    call print_string

    mov rax, [bytes_total]
    shr rax, 20
    mov [mib_last], rax
    call print_u64
    mov rsi, mib_in_msg
    call print_string

    mov rax, [cycles_last]
    mov rbx, 1000
    mul rbx
    mov rbx, [cycles_per_second]
    test rbx, rbx
    jnz .cps_ok_ms
    inc rbx
.cps_ok_ms:
    div rbx
    call print_u64
    mov rsi, ms_msg
    call print_string

    mov rax, [mib_last]
    mov rbx, [cycles_per_second]
    mul rbx
    mov rbx, [cycles_last]
    test rbx, rbx
    jnz .cycles_ok_bw
    inc rbx
.cycles_ok_bw:
    div rbx
    call print_u64
    mov rsi, mibps_msg
    call print_string
    ret

apply_key:
    cmp al, 0x01
    je .quit
    cmp al, 0x10
    je .quit
    cmp al, 0x02
    je .region0
    cmp al, 0x03
    je .region1
    cmp al, 0x04
    je .region2
    cmp al, 0x39
    je .toggle_region
    cmp al, 0x13
    je .toggle_region
    cmp al, 0x32
    je .toggle_mode
    cmp al, 0x0d
    je .more_passes
    cmp al, 0x4e
    je .more_passes
    cmp al, 0x0c
    je .fewer_passes
    cmp al, 0x4a
    je .fewer_passes
    xor eax, eax
    ret
.region0:
    mov qword [active_region], 0
    call print_config
    xor eax, eax
    ret
.region1:
    mov qword [active_region], 1
    call print_config
    xor eax, eax
    ret
.region2:
    mov qword [active_region], 2
    call print_config
    xor eax, eax
    ret
.toggle_region:
    mov rax, [active_region]
    inc rax
    cmp rax, 3
    jb .toggle_store
    xor eax, eax
.toggle_store:
    mov [active_region], rax
    call print_config
    xor eax, eax
    ret
.toggle_mode:
    xor qword [active_mode], 1
    call print_config
    xor eax, eax
    ret
.more_passes:
    mov rax, [passes_per_report]
    cmp rax, 1048576
    jae .more_done
    shl rax, 1
    mov [passes_per_report], rax
.more_done:
    call print_config
    xor eax, eax
    ret
.fewer_passes:
    mov rax, [passes_per_report]
    cmp rax, 1
    jbe .fewer_done
    shr rax, 1
    mov [passes_per_report], rax
.fewer_done:
    call print_config
    xor eax, eax
    ret
.quit:
    mov eax, 1
    ret

poll_key:
    xor eax, eax
    in al, 0x64
    test al, 1
    jz .none
    in al, 0x60
    test al, 0x80
    jnz .none
    ret
.none:
    xor eax, eax
    ret

rdtsc64:
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret

get_region_bounds:
    cmp rdi, 0
    je .low
    cmp rdi, 1
    je .mid
    mov r8, 0x0000000240000000
    mov r9, 0x0000000300000000
    ret
.low:
    mov r8, 0x0000000040000000
    mov r9, 0x00000000c0000000
    ret
.mid:
    mov r8, 0x0000000100000000
    mov r9, 0x00000001c0000000
    ret

conventional_bytes:
    push rbx
    push r12
    call get_region_bounds
    xor r12, r12
    movzx ecx, word [e820_count]
    mov rbx, e820_buffer
.loop:
    test ecx, ecx
    jz .done
    cmp dword [rbx + 16], 1
    jne .next
    mov r10, [rbx]
    mov r11, [rbx + 8]
    test r11, r11
    jz .next
    add r11, r10
    cmp r10, r8
    jae .start_ok
    mov r10, r8
.start_ok:
    cmp r11, r9
    jbe .end_ok
    mov r11, r9
.end_ok:
    cmp r11, r10
    jbe .next
    mov rax, r11
    sub rax, r10
    add r12, rax
.next:
    add rbx, e820_entry_size
    dec ecx
    jmp .loop
.done:
    mov rax, r12
    pop r12
    pop rbx
    ret

write_region:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r13, rsi
    call get_region_bounds
    mov r15, r8
    mov r14, r9
    xor r12, r12
    movzx r8d, word [e820_count]
    mov rbx, e820_buffer
.entry_loop:
    test r8d, r8d
    jz .done
    cmp dword [rbx + 16], 1
    jne .next
    mov r10, [rbx]
    mov r11, [rbx + 8]
    test r11, r11
    jz .next
    add r11, r10
    cmp r10, r15
    jae .chunk_start_ok
    mov r10, r15
.chunk_start_ok:
    cmp r11, r14
    jbe .chunk_end_ok
    mov r11, r14
.chunk_end_ok:
    cmp r11, r10
    jbe .next
    test r13, r13
    jnz .skip64_write64

.linear:
    add r10, 7
    and r10, -8
    and r11, -8
    cmp r11, r10
    jbe .next
    mov rcx, r11
    sub rcx, r10
    add r12, rcx
    shr rcx, 3
    mov rdi, r10
    mov rax, pattern
    cld
    rep stosq
    jmp .next

.skip64_write64:
    add r10, 63
    and r10, -64
    mov rax, r10
    sub rax, r15
    and rax, 127
    cmp rax, 64
    je .skip_loop
    jb .phase_before
    mov rdx, 192
    sub rdx, rax
    add r10, rdx
    jmp .skip_loop
.phase_before:
    mov rdx, 64
    sub rdx, rax
    add r10, rdx
.skip_loop:
    mov rax, r10
    add rax, 64
    cmp rax, r11
    ja .next
    mov rdi, r10
    mov rcx, 8
    mov rax, pattern
    cld
    rep stosq
    add r12, 64
    add r10, 128
    jmp .skip_loop

.next:
    add rbx, e820_entry_size
    dec r8d
    jmp .entry_loop
.done:
    sfence
    mov rax, r12
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

print_region_name:
    test rdi, rdi
    jnz .not_low
    mov rsi, region_low_msg
    jmp print_string
.not_low:
    cmp rdi, 1
    jne .high
    mov rsi, region_mid_msg
    jmp print_string
.high:
    mov rsi, region_high_msg
    jmp print_string

print_mode_name:
    test rdi, rdi
    jnz .skip
    mov rsi, mode_linear_msg
    jmp print_string
.skip:
    mov rsi, mode_skip_msg
    jmp print_string

clear_screen:
    cld
    mov rdi, 0xb8000
    mov rcx, 80 * 25
    mov ax, 0x0720
    rep stosw
    mov qword [cursor_pos], 0
    ret

scroll_if_needed:
    cmp qword [cursor_pos], 80 * 25
    jb .done
    push rax
    push rcx
    push rsi
    push rdi
    cld
    mov rsi, 0xb8000 + 160
    mov rdi, 0xb8000
    mov rcx, 80 * 24
    rep movsw
    mov rdi, 0xb8000 + (80 * 24 * 2)
    mov rcx, 80
    mov ax, 0x0720
    rep stosw
    mov qword [cursor_pos], 80 * 24
    pop rdi
    pop rsi
    pop rcx
    pop rax
.done:
    ret

print_char:
    cmp al, 10
    je .newline
    cmp al, 13
    je .done
    push rax
    call serial_put_al
    pop rax
    mov rbx, [cursor_pos]
    mov ah, 0x07
    mov [0xb8000 + rbx * 2], ax
    inc qword [cursor_pos]
    call scroll_if_needed
.done:
    ret
.newline:
    push rax
    mov al, 13
    call serial_put_al
    mov al, 10
    call serial_put_al
    pop rax
    mov rax, [cursor_pos]
    xor rdx, rdx
    mov rcx, 80
    div rcx
    inc rax
    imul rax, rax, 80
    mov [cursor_pos], rax
    call scroll_if_needed
    ret

serial_put_al:
    push rax
    push rbx
    push rcx
    push rdx
    mov bl, al
    mov dx, 0x3fd
    mov rcx, 100000
.wait:
    in al, dx
    test al, 0x20
    jnz .ready
    loop .wait
    jmp .done
.ready:
    mov dx, 0x3f8
    mov al, bl
    out dx, al
.done:
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

print_string:
    push rax
    push rsi
.loop:
    lodsb
    test al, al
    jz .done
    call print_char
    jmp .loop
.done:
    pop rsi
    pop rax
    ret

print_u64:
    push rbx
    push rcx
    push rdx
    push rdi
    test rax, rax
    jnz .convert
    mov al, '0'
    call print_char
    jmp .done
.convert:
    mov rdi, number_buffer + 32
    mov rbx, 10
.digit_loop:
    xor rdx, rdx
    div rbx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    test rax, rax
    jnz .digit_loop
.print_loop:
    mov al, [rdi]
    cmp rdi, number_buffer + 32
    jae .done
    call print_char
    inc rdi
    jmp .print_loop
.done:
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    ret

align 8
e820_count:
    dw 0
tsc_start_low:
    dd 0
tsc_start_high:
    dd 0
tsc_200ms_delta:
    dq 0
cycles_per_second:
    dq 1
tsc_start:
    dq 0
cycles_last:
    dq 1
bytes_total:
    dq 0
passes_done:
    dq 0
passes_per_report:
    dq 1
active_region:
    dq 0
active_mode:
    dq 0
run_region:
    dq 0
run_mode:
    dq 0
run_ppr:
    dq 1
report_no:
    dq 1
mib_last:
    dq 0
cursor_pos:
    dq 0
pending_key:
    db 0

header_msg:
    db "EFIRAM BIOS RAM write test", 10
    db "Pattern: 0xa5a5a5a55a5a5a5a", 10
    db "Keys: 1=1-3 GiB, 2=4-7 GiB, 3=9-12 GiB, space/r=toggle region, m=toggle mode, +/-=passes per print, q/esc=quit", 10, 10, 0
e820_msg:
    db "E820 entries: ", 0
tsc_msg:
    db "Estimated TSC: ", 0
mhz_msg:
    db " MHz", 10, 10, 0
region_low_msg:
    db "1-3 GiB", 0
region_mid_msg:
    db "4-7 GiB", 0
region_high_msg:
    db "9-12 GiB", 0
mode_linear_msg:
    db "linear", 0
mode_skip_msg:
    db "skip64/write64", 0
available_mid_msg:
    db ": ", 0
available_end_msg:
    db " MiB usable RAM available in requested range", 10, 0
active_region_msg:
    db "Active region: ", 0
mode_prefix_msg:
    db ", mode: ", 0
passes_prefix_msg:
    db ", print every ", 0
pass_word:
    db " pass", 0
es_suffix:
    db "es", 0
report_msg:
    db "Report ", 0
colon_space:
    db ": ", 0
comma_space:
    db ", ", 0
wrote_msg:
    db ", wrote ", 0
mib_in_msg:
    db " MiB in ", 0
ms_msg:
    db " ms, ", 0
mibps_msg:
    db " MiB/s", 10, 0
stopped_msg:
    db 10, "Stopped.", 10, 0
newline:
    db 10, 0

align 16
number_buffer:
    times 32 db 0
