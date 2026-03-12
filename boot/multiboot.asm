; boot/multiboot.asm — Multiboot2 header and kernel entry point for MonoOS (Dori kernel)

section .multiboot
align 8

; Multiboot2 constants
MULTIBOOT2_MAGIC    equ 0xE85250D6
MULTIBOOT2_ARCH     equ 0              ; i386 protected mode
MULTIBOOT2_LENGTH   equ (multiboot_header_end - multiboot_header_start)
MULTIBOOT2_CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_LENGTH)

multiboot_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_LENGTH
    dd MULTIBOOT2_CHECKSUM

    ; End tag
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
multiboot_header_end:

; -------------------------------------------------------------------
section .bss
align 16

; Kernel stack — 16 KiB
stack_bottom:
    resb 16384
stack_top:

; -------------------------------------------------------------------
section .text
global _start
extern kernel_main

_start:
    ; Set up the stack
    mov esp, stack_top

    ; Push multiboot info pointer (ebx) and magic number (eax)
    push ebx    ; multiboot info struct pointer
    push eax    ; multiboot magic number

    ; Call the C kernel entry point
    call kernel_main

    ; If kernel_main returns, hang the CPU
    cli
.hang:
    hlt
    jmp .hang
