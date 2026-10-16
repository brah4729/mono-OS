; libc/crt0.S — C runtime startup for MonoOS userland programs (x86_64)

[BITS 64]
[GLOBAL _start]
[EXTERN main]

section .text

_start:
    xor rbp, rbp            ; clean stack traces
    and rsp, -16            ; SysV: 16-byte aligned before call
    xor edi, edi            ; argc = 0
    xor esi, esi            ; argv = NULL
    call main

    mov edi, eax            ; exit code
    xor eax, eax            ; SYS_EXIT = 0
    int 0x80

.hang:
    jmp .hang               ; hlt would #GP once userland runs in ring 3
