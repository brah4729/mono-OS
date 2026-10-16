; boot/idt.asm — IDT load, ISR and IRQ stubs

section .text
bits 64
global idt_flush
extern isr_handler
extern irq_handler

; void idt_flush(uint64_t idt_ptr)
idt_flush:
    lidt [rdi]
    ret

; ---------------------------------------------------------------
; Macro for ISRs that do NOT push an error code
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0           ; Push dummy error code
    push qword %1          ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro for ISRs that DO push an error code
%macro ISR_ERR 1
global isr%1
isr%1:
    push qword %1          ; Push interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; Macro for IRQ stubs
%macro IRQ 2
global irq%1
irq%1:
    push qword 0           ; Push dummy error code
    push qword %2          ; Push interrupt number (32 + IRQ#)
    jmp irq_common_stub
%endmacro

; ---------------------------------------------------------------
; ISR stubs 0-31
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

; IRQ stubs 0-15 (mapped to interrupts 32-47)
IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; ---------------------------------------------------------------
; System call stub (int 0x80)
ISR_NOERR 128

; ---------------------------------------------------------------
%macro PUSH_ALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_ALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

; Stack at call: CPU frame (40) + err/int (16) + 15 GPRs (120) = 176 bytes = 16-aligned,
; because the CPU aligns RSP to 16 before pushing the frame in long mode.
isr_common_stub:
    PUSH_ALL
    cld
    mov rdi, rsp                    ; registers_t*
    call isr_handler
    POP_ALL
    add rsp, 16                     ; drop int_no + err_code
    iretq

irq_common_stub:
    PUSH_ALL
    cld
    mov rdi, rsp
    call irq_handler
    POP_ALL
    add rsp, 16
    iretq
