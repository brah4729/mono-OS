; boot/context_switch.asm — Process context switch routine (x86_64 SysV)
;
; void context_switch(uint64_t* old_rsp, uint64_t new_rsp);
;   rdi = &old_rsp, rsi = new_rsp
; Saves callee-saved rbp, rbx, r12–r15, swaps stacks, restores, returns.

section .text
bits 64
global context_switch

context_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp
    mov rsp, rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
