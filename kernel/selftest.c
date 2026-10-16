/* kernel/selftest.c — boot-time self-tests, results logged to serial as [TEST] lines */

#include "../include/selftest.h"
#include "../include/serial.h"
#include "../include/string.h"
#include "../include/isr.h"
#include "../include/idt.h"
#include "../include/gdt.h"
#include "../include/pit.h"
#include "../include/io.h"

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
