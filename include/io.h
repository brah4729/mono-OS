#ifndef IO_H
#define IO_H

#include "types.h"

/* Write a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a byte from an I/O port */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Write a word to an I/O port */
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a word from an I/O port */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Wait for an I/O operation to complete */
static inline void io_wait(void) {
    outb(0x80, 0);
}

/* Enable interrupts */
static inline void sti(void) {
    __asm__ volatile ("sti");
}

/* Disable interrupts */
static inline void cli(void) {
    __asm__ volatile ("cli");
}

/* Halt the CPU */
static inline void hlt(void) {
    __asm__ volatile ("hlt");
}

#endif
