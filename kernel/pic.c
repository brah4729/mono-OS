/* kernel/pic.c — 8259 Programmable Interrupt Controller driver */

#include "../include/pic.h"
#include "../include/io.h"

/* ICW1 flags */
#define ICW1_ICW4       0x01
#define ICW1_INIT       0x10

/* ICW4 flags */
#define ICW4_8086       0x01

void pic_init(void) {
    uint8_t mask1, mask2;

    /* Save current masks */
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    /* ICW1: Start initialization sequence (cascade mode) */
    outb(PIC1_CMD,  ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD,  ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: Remap IRQs — master: 32-39, slave: 40-47 */
    outb(PIC1_DATA, 0x20);  /* Master PIC vector offset (IRQ 0-7 → INT 32-39) */
    io_wait();
    outb(PIC2_DATA, 0x28);  /* Slave PIC vector offset  (IRQ 8-15 → INT 40-47) */
    io_wait();

    /* ICW3: Tell master about slave on IRQ2, tell slave its cascade identity */
    outb(PIC1_DATA, 0x04);  /* Slave PIC at IRQ2 (bit 2) */
    io_wait();
    outb(PIC2_DATA, 0x02);  /* Slave cascade identity */
    io_wait();

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Restore saved masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
