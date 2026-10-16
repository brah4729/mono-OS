; boot/gdt.asm — GDT and TSS load routines (x86_64)

section .text
bits 64
global gdt_flush
global tss_flush

; void gdt_flush(uint64_t gdt_ptr)
gdt_flush:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rax                 ; return address
    push qword 0x08         ; new CS
    push rax
    retfq                   ; far return reloads CS

; void tss_flush(void)
tss_flush:
    mov ax, 0x28
    ltr ax
    ret
