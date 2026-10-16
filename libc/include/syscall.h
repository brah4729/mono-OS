#ifndef _MONO_SYSCALL_H
#define _MONO_SYSCALL_H

/*
 * monolibc — internal syscall interface
 *
 * Register convention (Linux x86_64 registers, entered via int 0x80):
 *   rax = syscall number
 *   rdi = arg1, rsi = arg2, rdx = arg3
 *   r10 = arg4, r8  = arg5
 *   rax = return value (negative = error)
 */

/* Syscall numbers — must match kernel/include/syscall.h */
#define _SYS_EXIT       0
#define _SYS_READ       1
#define _SYS_WRITE      2
#define _SYS_OPEN       3
#define _SYS_CLOSE      4
#define _SYS_STAT       5
#define _SYS_SEEK       6
#define _SYS_READDIR    7
#define _SYS_EXEC       8
#define _SYS_FORK       9
#define _SYS_WAITPID    10
#define _SYS_BRK        11
#define _SYS_GETPID     12
#define _SYS_MKDIR      13
#define _SYS_UNLINK     14
#define _SYS_SYMLINK    15
#define _SYS_READLINK   16
#define _SYS_GETCWD     17
#define _SYS_CHDIR      18

/* Inline syscall wrappers using int 0x80 */

static inline long _syscall0(long num) {
    long ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num) : "memory");
    return ret;
}

static inline long _syscall1(long num, long a1) {
    long ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num), "D"(a1) : "memory");
    return ret;
}

static inline long _syscall2(long num, long a1, long a2) {
    long ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2) : "memory");
    return ret;
}

static inline long _syscall3(long num, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2), "d"(a3) : "memory");
    return ret;
}

#endif
