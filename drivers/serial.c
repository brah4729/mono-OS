/* drivers/serial.c — COM1 serial port driver for debug output */

#include "../include/serial.h"
#include "../include/io.h"
#include "../include/string.h"

static int serial_is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00);  /* Disable all interrupts */
    outb(COM1 + 3, 0x80);  /* Enable DLAB (set baud rate divisor) */
    outb(COM1 + 0, 0x03);  /* Set divisor to 3 (lo byte) — 38400 baud */
    outb(COM1 + 1, 0x00);  /*                  (hi byte) */
    outb(COM1 + 3, 0x03);  /* 8 bits, no parity, one stop bit */
    outb(COM1 + 2, 0xC7);  /* Enable FIFO, clear them, 14-byte threshold */
    outb(COM1 + 4, 0x0B);  /* IRQs enabled, RTS/DSR set */
}

void serial_putchar(char c) {
    while (!serial_is_transmit_empty());
    outb(COM1, c);
}

void serial_puts(const char* str) {
    while (*str) {
        if (*str == '\n') serial_putchar('\r');
        serial_putchar(*str++);
    }
}

void serial_put_hex(uint32_t value) {
    char buf[16];
    utoa(value, buf, 16);
    serial_puts(buf);
}
