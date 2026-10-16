/* kernel/selftest.c — boot-time self-tests, results logged to serial as [TEST] lines */

#include "../include/selftest.h"
#include "../include/serial.h"
#include "../include/string.h"
#include "../include/isr.h"
#include "../include/idt.h"
#include "../include/gdt.h"
#include "../include/pit.h"
#include "../include/io.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/heap.h"
#include "../include/process.h"
#include "../include/syscall.h"
#include "../include/elf.h"
#include "../include/dorifs.h"

_Static_assert(sizeof(registers_t) == 22 * 8, "registers_t must match boot/idt.asm push order");
_Static_assert(sizeof(struct idt_entry) == 16, "x86_64 IDT gate is 16 bytes");
_Static_assert(sizeof(struct tss_entry) == 104, "x86_64 TSS is 104 bytes");

void selftest_report(const char* name, bool ok) {
    serial_puts("[TEST] ");
    serial_puts(name);
    serial_puts(ok ? " ok\n" : " FAIL\n");
}

/* ─── utoa 64-bit (Review Focus #2) ─────────────────────── */

static void test_utoa64(void) {
    char buf[24];
    utoa(0xFFFFFFFFFFFFFFFFUL, buf, 16);
    bool hex_ok = strcmp(buf, "FFFFFFFFFFFFFFFF") == 0 || strcmp(buf, "ffffffffffffffff") == 0;
    utoa(18446744073709551615UL, buf, 10);
    bool dec_ok = strcmp(buf, "18446744073709551615") == 0;
    selftest_report("utoa64", hex_ok && dec_ok);
}

/* ─── int3 round trip through the IDT ───────────────────── */

static volatile int int3_seen = 0;
static volatile uint64_t int3_rip = 0;

static void int3_handler(registers_t* regs) {
    int3_seen = 1;
    int3_rip = regs->rip;
}

static void test_int3(void) {
    isr_register_handler(3, int3_handler);
    __asm__ volatile ("int3");
    selftest_report("idt-int3", int3_seen == 1 && int3_rip != 0);
    isr_register_handler(3, 0);
}

/* ─── PIT ticks advance with interrupts on ──────────────── */

static void test_pit(void) {
    uint32_t start = pit_get_ticks();
    for (volatile uint64_t spin = 0; spin < 200000000UL; spin++) {
        if (pit_get_ticks() - start >= 10) break;
    }
    selftest_report("pit-ticks", pit_get_ticks() - start >= 10);
}

/* ─── Segment registers hold the new GDT selectors ──────── */

static void test_segments(void) {
    uint16_t cs, ds, tr;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile ("mov %%ds, %0" : "=r"(ds));
    __asm__ volatile ("str %0" : "=r"(tr));
    selftest_report("gdt-selectors", cs == 0x08 && ds == 0x10 && tr == 0x28);
}

void selftest_cpu(void) {
    test_utoa64();
    test_segments();
    test_int3();
    test_pit();
}

/* ─── Memory: VMM above 4 GiB, huge-page split, heap ────── */

static void test_vmm_high(void) {
    const uint64_t virt = 0x0000400000000000UL;   /* far outside the boot identity map */
    uint64_t frame = pmm_alloc_frame();
    if (!frame) { selftest_report("vmm-high-map", false); return; }

    vmm_map_page(frame, virt, PAGE_WRITABLE);
    *(volatile uint64_t*)virt = 0xDEADBEEFCAFEBABEUL;
    bool ok = *(volatile uint64_t*)(uintptr_t)frame == 0xDEADBEEFCAFEBABEUL
           && vmm_get_physical(virt) == frame;
    vmm_unmap_page(virt);
    ok = ok && vmm_get_physical(virt) == 0;
    pmm_free_frame(frame);
    selftest_report("vmm-high-map", ok);
}

/* Review Focus #3: remapping inside a boot 2 MiB page must keep its neighbours identity-mapped */
static void test_vmm_split(void) {
    const uint64_t virt = 0x400000UL;             /* ELF load address, inside boot PD entry 2 */
    const uint64_t neighbour = 0x5FF000UL;        /* same 2 MiB page, different 4 KiB page */
    uint64_t frame = pmm_alloc_frame();
    if (!frame) { selftest_report("vmm-huge-split", false); return; }

    vmm_map_page(frame, virt, PAGE_WRITABLE);
    *(volatile uint32_t*)virt = 0x12345678;
    bool ok = *(volatile uint32_t*)(uintptr_t)frame == 0x12345678
           && vmm_get_physical(neighbour) == neighbour;
    vmm_map_page(virt, virt, PAGE_WRITABLE);      /* restore identity */
    pmm_free_frame(frame);
    selftest_report("vmm-huge-split", ok);
}

static void test_heap(void) {
    void* a = kmalloc(24);
    void* b = kmalloc(3 * 1024 * 1024);           /* backbuffer-sized */
    void* c = kmalloc(1);
    bool ok = a && b && c
           && ((uintptr_t)a & 0xF) == 0
           && ((uintptr_t)b & 0xF) == 0
           && ((uintptr_t)c & 0xF) == 0;
    if (b) memset(b, 0xAB, 3 * 1024 * 1024);      /* must not fault or corrupt c */
    kfree(a); kfree(b); kfree(c);
    void* d = kmalloc(24);
    ok = ok && d != 0;
    kfree(d);
    selftest_report("heap-align", ok);
}

void selftest_memory(void) {
    test_vmm_high();
    test_vmm_split();
    test_heap();
}

/* ─── Context switch round trip + new-thread stack alignment ─ */

static uint64_t main_rsp, co_rsp;
static volatile int co_ran = 0;
static volatile int co_aligned = 0;

__attribute__((noinline)) static void co_entry(void) {
    co_aligned = (((uintptr_t)__builtin_frame_address(0)) & 0xF) == 0;
    co_ran = 1;
    context_switch(&co_rsp, main_rsp);
}

void selftest_sched(void) {
    const size_t size = 8192;
    uint8_t* stack = kmalloc(size);
    if (!stack) { selftest_report("ctx-switch", false); return; }

    uint64_t* sp = (uint64_t*)(((uintptr_t)stack + size) & ~(uintptr_t)0xF);
    *(--sp) = 0;                          /* fake return address → entry sees rsp ≡ 8 (mod 16) */
    *(--sp) = (uint64_t)(uintptr_t)co_entry;
    for (int i = 0; i < 6; i++) *(--sp) = 0;   /* rbp rbx r12 r13 r14 r15 */
    co_rsp = (uint64_t)(uintptr_t)sp;

    context_switch(&main_rsp, co_rsp);

    selftest_report("ctx-switch", co_ran == 1);
    selftest_report("ctx-stack-align", co_aligned == 1);
    kfree(stack);
}

_Static_assert(sizeof(dorifs_superblock_t) == 4096, "DoriFS superblock layout changed — disk images break");
_Static_assert(sizeof(dorifs_inode_t) == 344,       "DoriFS inode layout changed — disk images break");

/* ─── Syscall ABI: rax = nr, rdi/rsi/rdx args, rax = ret ── */

static long raw_syscall1(long nr, long a1) {
    long ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(nr), "D"(a1) : "memory");
    return ret;
}

void selftest_syscall(void) {
    long pid = raw_syscall1(SYS_GETPID, 0);
    long bad = raw_syscall1(999, 0);
    selftest_report("syscall-getpid", pid == (long)process_get_pid());
    selftest_report("syscall-invalid", bad == -1);
}

/* ─── ELF64 header validation ───────────────────────────── */

void selftest_elf(void) {
    elf64_ehdr_t h;
    memset(&h, 0, sizeof(h));
    h.e_ident[0] = 0x7F; h.e_ident[1] = 'E'; h.e_ident[2] = 'L'; h.e_ident[3] = 'F';
    h.e_ident[4] = ELFCLASS64;
    h.e_ident[5] = ELFDATA2LSB;
    h.e_ident[6] = 1;
    h.e_type = ET_EXEC;
    h.e_machine = EM_X86_64;
    h.e_version = 1;
    h.e_phentsize = sizeof(elf64_phdr_t);
    bool good = elf_validate(&h, sizeof(h)) == 0;

    h.e_ident[4] = 1;                 /* ELFCLASS32 → must be rejected */
    bool rej32 = elf_validate(&h, sizeof(h)) == -1;
    h.e_ident[4] = ELFCLASS64;
    h.e_machine = 3;                  /* EM_386 → must be rejected */
    bool rej386 = elf_validate(&h, sizeof(h)) == -1;
    bool rejshort = elf_validate(&h, 10) == -1;

    selftest_report("elf64-validate", good && rej32 && rej386 && rejshort);
}
