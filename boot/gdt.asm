; boot/gdt.asm — GDT and TSS flush routines

section .text
global gdt_flush
global tss_flush

; void gdt_flush(uint32_t gdt_ptr)
gdt_flush:
    mov eax, [esp + 4]     ; Get the pointer to the GDT
    lgdt [eax]             ; Load the new GDT

    ; Reload segment registers
    mov ax, 0x10           ; 0x10 = kernel data segment (entry 2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload CS with kernel code segment (0x08 = entry 1)
    jmp 0x08:.flush
.flush:
    ret

; void tss_flush(void)
tss_flush:
    mov ax, 0x2B           ; TSS segment selector (entry 5 | RPL 3)
    ltr ax
    ret
