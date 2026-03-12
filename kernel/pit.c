/* kernel/pit.c — Programmable Interval Timer (channel 0) */

#include "../include/pit.h"
#include "../include/isr.h"
#include "../include/io.h"
#include "../include/serial.h"

static volatile uint32_t tick_count = 0;
static uint32_t pit_hz = 0;

static void pit_callback(registers_t* regs) {
    (void)regs;
    tick_count++;
}

void pit_init(uint32_t frequency) {
    pit_hz = frequency;
    tick_count = 0;

    uint32_t divisor = PIT_FREQ / frequency;

    /* Channel 0, lobyte/hibyte, rate generator */
    outb(PIT_CMD, 0x36);

    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));        /* Low byte */
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF)); /* High byte */

    irq_register_handler(0, pit_callback);

    serial_puts("[PIT] Initialized at ");
    char buf[16];
    utoa(frequency, buf, 10);
    serial_puts(buf);
    serial_puts(" Hz\n");
}

uint32_t pit_get_ticks(void) {
    return tick_count;
}

void pit_sleep(uint32_t ms) {
    uint32_t target = tick_count + (ms * pit_hz / 1000);
    while (tick_count < target) {
        __asm__ volatile ("hlt");
    }
}
