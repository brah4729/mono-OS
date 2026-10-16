/* kernel/syscall.c — System call dispatcher */

#include "../include/syscall.h"
#include "../include/vfs.h"
#include "../include/process.h"
#include "../include/serial.h"
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/idt.h"

/* Forward declarations for syscall implementations */
static int64_t sys_exit(registers_t* regs);
static int64_t sys_read(registers_t* regs);
static int64_t sys_write(registers_t* regs);
static int64_t sys_open(registers_t* regs);
static int64_t sys_close(registers_t* regs);
static int64_t sys_stat(registers_t* regs);
static int64_t sys_seek(registers_t* regs);
static int64_t sys_readdir(registers_t* regs);
static int64_t sys_exec(registers_t* regs);
static int64_t sys_fork(registers_t* regs);
static int64_t sys_waitpid(registers_t* regs);
static int64_t sys_brk(registers_t* regs);
static int64_t sys_getpid(registers_t* regs);
static int64_t sys_mkdir(registers_t* regs);
static int64_t sys_unlink(registers_t* regs);
static int64_t sys_symlink(registers_t* regs);
static int64_t sys_readlink(registers_t* regs);
static int64_t sys_getcwd(registers_t* regs);
static int64_t sys_chdir(registers_t* regs);

/* Syscall dispatch table */
typedef int64_t (*syscall_fn_t)(registers_t*);

static syscall_fn_t syscall_table[SYSCALL_COUNT] = {
    [SYS_EXIT]     = sys_exit,
    [SYS_READ]     = sys_read,
    [SYS_WRITE]    = sys_write,
    [SYS_OPEN]     = sys_open,
    [SYS_CLOSE]    = sys_close,
    [SYS_STAT]     = sys_stat,
    [SYS_SEEK]     = sys_seek,
    [SYS_READDIR]  = sys_readdir,
    [SYS_EXEC]     = sys_exec,
    [SYS_FORK]     = sys_fork,
    [SYS_WAITPID]  = sys_waitpid,
    [SYS_BRK]      = sys_brk,
    [SYS_GETPID]   = sys_getpid,
    [SYS_MKDIR]    = sys_mkdir,
    [SYS_UNLINK]   = sys_unlink,
    [SYS_SYMLINK]  = sys_symlink,
    [SYS_READLINK] = sys_readlink,
    [SYS_GETCWD]   = sys_getcwd,
    [SYS_CHDIR]    = sys_chdir,
};

/* ─── Syscall dispatcher ────────────────────────────────── */

void syscall_handler(registers_t* regs) {
    if (regs->rax >= SYSCALL_COUNT || !syscall_table[regs->rax]) {
        serial_puts("[SYSCALL] Invalid syscall: ");
        char buf[24];
        utoa(regs->rax, buf, 10);
        serial_puts(buf);
        serial_puts("\n");
        regs->rax = (uint64_t)-1;
        return;
    }
    int64_t result = syscall_table[regs->rax](regs);
    regs->rax = (uint64_t)result;
}

void syscall_init(void) {
    isr_register_handler(SYSCALL_INT, syscall_handler);
    serial_puts("[SYSCALL] System call handler registered at int 0x80\n");
}

/* ─── Syscall implementations ──────────────────────────── */

/*
 * Register convention (Linux x86_64 registers, entered via int 0x80):
 *   rax = syscall number
 *   rdi = arg1
 *   rsi = arg2
 *   rdx = arg3
 *   r10 = arg4
 *   r8  = arg5
 *   rax = return value (negative = error)
 */

static int64_t sys_exit(registers_t* regs) {
    int32_t status = (int32_t)regs->rdi;
    serial_puts("[SYSCALL] exit(");
    char buf[24];
    itoa(status, buf, 10);
    serial_puts(buf);
    serial_puts(")\n");
    process_exit(status);
    return 0; /* Never reached */
}

static int64_t sys_read(registers_t* regs) {
    int fd = (int)regs->rdi;
    void* buffer = (void*)(uintptr_t)regs->rsi;
    uint32_t size = regs->rdx;
    return (int32_t)vfs_read(fd, buffer, size);
}

static int64_t sys_write(registers_t* regs) {
    int fd = (int)regs->rdi;
    const void* buffer = (const void*)(uintptr_t)regs->rsi;
    uint32_t size = regs->rdx;

    /* Special case: fd 1 (stdout) and fd 2 (stderr) write to VGA */
    if (fd == 1 || fd == 2) {
        const char* str = (const char*)buffer;
        for (uint32_t i = 0; i < size; i++) {
            vga_putchar(str[i]);
        }
        return (int32_t)size;
    }

    return (int32_t)vfs_write(fd, buffer, size);
}

static int64_t sys_open(registers_t* regs) {
    const char* path = (const char*)(uintptr_t)regs->rdi;
    uint32_t flags = regs->rsi;
    return vfs_open(path, flags);
}

static int64_t sys_close(registers_t* regs) {
    int fd = (int)regs->rdi;
    return vfs_close(fd);
}

static int64_t sys_stat(registers_t* regs) {
    const char* path = (const char*)(uintptr_t)regs->rdi;
    vfs_stat_t* st = (vfs_stat_t*)(uintptr_t)regs->rsi;
    return vfs_stat(path, st);
}

static int64_t sys_seek(registers_t* regs) {
    int fd = (int)regs->rdi;
    int32_t offset = (int32_t)regs->rsi;
    int whence = (int)regs->rdx;
    return vfs_seek(fd, offset, whence);
}

static int64_t sys_readdir(registers_t* regs) {
    const char* path = (const char*)(uintptr_t)regs->rdi;
    uint32_t index = regs->rsi;
    vfs_dirent_t* out = (vfs_dirent_t*)(uintptr_t)regs->rdx;
    return vfs_readdir(path, index, out);
}

static int64_t sys_exec(registers_t* regs) {
    const char* path = (const char*)(uintptr_t)regs->rdi;
    (void)path;
    /* TODO: Implement exec with ELF loader */
    serial_puts("[SYSCALL] exec() not yet implemented\n");
    return -1;
}

static int64_t sys_fork(registers_t* regs) {
    (void)regs;
    /* TODO: Implement fork */
    serial_puts("[SYSCALL] fork() not yet implemented\n");
    return -1;
}

static int64_t sys_waitpid(registers_t* regs) {
    (void)regs;
    /* TODO: Implement waitpid */
    serial_puts("[SYSCALL] waitpid() not yet implemented\n");
    return -1;
}

static int64_t sys_brk(registers_t* regs) {
    (void)regs;
    /* TODO: Implement brk for user-space heap */
    serial_puts("[SYSCALL] brk() not yet implemented\n");
    return -1;
}

static int64_t sys_getpid(registers_t* regs) {
    (void)regs;
    return (int32_t)process_get_pid();
}

static int64_t sys_mkdir(registers_t* regs) {
    const char* path = (const char*)(uintptr_t)regs->rdi;
    return vfs_mkdir(path);
}

static int64_t sys_unlink(registers_t* regs) {
    const char* path = (const char*)(uintptr_t)regs->rdi;
    return vfs_unlink(path);
}

static int64_t sys_symlink(registers_t* regs) {
    const char* target = (const char*)(uintptr_t)regs->rdi;
    const char* linkpath = (const char*)(uintptr_t)regs->rsi;
    return vfs_symlink(target, linkpath);
}

static int64_t sys_readlink(registers_t* regs) {
    const char* path = (const char*)(uintptr_t)regs->rdi;
    char* buf = (char*)(uintptr_t)regs->rsi;
    size_t bufsiz = (size_t)regs->rdx;
    return (int32_t)vfs_readlink(path, buf, bufsiz);
}

static int64_t sys_getcwd(registers_t* regs) {
    (void)regs;
    /* TODO: Track per-process working directory */
    return -1;
}

static int64_t sys_chdir(registers_t* regs) {
    (void)regs;
    /* TODO: Change per-process working directory */
    return -1;
}
