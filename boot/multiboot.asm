; boot/multiboot.asm — Multiboot2 header, 32→64-bit trampoline, kernel entry (x86_64)
;
; GRUB enters _start in 32-bit protected mode, paging off, eax = MB2 magic,
; ebx = MB2 info physical address. We identity-map 0–4 GiB with 2 MiB pages,
; enable long mode, and call kernel_main(rdi = magic, rsi = mbi).

section .multiboot
align 8
MULTIBOOT2_MAGIC    equ 0xE85250D6
MULTIBOOT2_ARCH     equ 0              ; i386 protected-mode entry (GRUB has no 64-bit entry)
MULTIBOOT2_LENGTH   equ (multiboot_header_end - multiboot_header_start)
MULTIBOOT2_CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_LENGTH)

multiboot_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_LENGTH
    dd MULTIBOOT2_CHECKSUM

    ; ─── Framebuffer request tag ───
    align 8
    dw 5
    dw 0
    dd 20
    dd 1024
    dd 768
    dd 32

    ; ─── End tag ───
    align 8
    dw 0
    dw 0
    dd 8
multiboot_header_end:

; -------------------------------------------------------------------
section .bss
align 4096
boot_pml4:  resb 4096
boot_pdpt:  resb 4096
boot_pd:    resb 4096 * 4          ; 4 PDs × 512 × 2 MiB = 4 GiB
align 16
stack_bottom:
    resb 16384
stack_top:

; -------------------------------------------------------------------
section .rodata
align 8
boot_gdt64:
    dq 0                            ; 0x00 null
    dq 0x00AF9A000000FFFF           ; 0x08 kernel code, L=1
    dq 0x00CF92000000FFFF           ; 0x10 kernel data
boot_gdt64_end:
boot_gdt64_ptr:
    dw boot_gdt64_end - boot_gdt64 - 1
    dq boot_gdt64

; -------------------------------------------------------------------
section .text
bits 32
global _start
extern kernel_main

COM1 equ 0x3F8

_start:
    mov esp, stack_top
    mov edi, eax                    ; save MB2 magic (kernel_main arg 1)
    mov esi, ebx                    ; save MB2 info  (kernel_main arg 2)

    call check_long_mode
    call setup_page_tables

    mov eax, cr4                    ; PAE
    or eax, 1 << 5
    mov cr4, eax

    mov eax, boot_pml4
    mov cr3, eax

    mov ecx, 0xC0000080             ; EFER
    rdmsr
    or eax, 1 << 8                  ; LME
    wrmsr

    mov eax, cr0                    ; PG (PE already set by GRUB)
    or eax, 1 << 31
    mov cr0, eax

    lgdt [boot_gdt64_ptr]
    jmp 0x08:long_mode_start

; Halts with "NO64" on VGA + serial if the CPU lacks long mode.
; Clobbers eax, ebx, ecx, edx (edi/esi preserved).
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov dword [0xB8000], 0x4F4F4F4E ; "NO" white-on-red
    mov dword [0xB8004], 0x4F344F36 ; "64"
    mov ebx, no64_msg
.serial_loop:
    mov al, [ebx]
    test al, al
    jz .halt
    mov dx, COM1
    out dx, al
    inc ebx
    jmp .serial_loop
.halt:
    cli
    hlt
    jmp .halt

; Identity-map 0–4 GiB: PML4[0] → PDPT, PDPT[0..3] → PD[0..3], 2048 × 2 MiB pages.
; Clobbers eax, ecx.
setup_page_tables:
    mov eax, boot_pdpt
    or eax, 0b11                    ; present | writable
    mov [boot_pml4], eax

    xor ecx, ecx
.pdpt_loop:
    mov eax, ecx
    shl eax, 12
    add eax, boot_pd
    or eax, 0b11
    mov [boot_pdpt + ecx * 8], eax
    inc ecx
    cmp ecx, 4
    jne .pdpt_loop

    xor ecx, ecx
.pd_loop:
    mov eax, ecx
    shl eax, 21                     ; ecx × 2 MiB
    or eax, 0b10000011              ; present | writable | huge
    mov [boot_pd + ecx * 8], eax
    mov dword [boot_pd + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 2048
    jne .pd_loop
    ret

section .rodata
no64_msg: db "[BOOT] no long mode: CPU is not x86_64", 10, 0

; -------------------------------------------------------------------
section .text
bits 64
long_mode_start:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov edi, edi                    ; zero-extend magic (upper half undefined after mode switch)
    mov esi, esi                    ; zero-extend MB2 info address
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
