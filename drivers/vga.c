/* drivers/vga.c — VGA text-mode driver (80x25, color, scrolling) */

#include "../include/vga.h"
#include "../include/io.h"
#include "../include/string.h"

static uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;

static int vga_row = 0;
static int vga_col = 0;
static uint8_t vga_color_attr = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)(uint8_t)c | ((uint16_t)color << 8);
}

static inline uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static void vga_update_cursor(void) {
    uint16_t pos = vga_row * VGA_WIDTH + vga_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_scroll(void) {
    if (vga_row >= VGA_HEIGHT) {
        /* Move everything up one row */
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            VGA_MEMORY[i] = VGA_MEMORY[i + VGA_WIDTH];
        }
        /* Clear the last row */
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            VGA_MEMORY[i] = vga_entry(' ', vga_color_attr);
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

void vga_init(void) {
    vga_color_attr = vga_make_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = vga_entry(' ', vga_color_attr);
    }
    vga_row = 0;
    vga_col = 0;
    vga_update_cursor();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color_attr = vga_make_color(fg, bg);
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', vga_color_attr);
        }
    } else {
        VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_color_attr);
        vga_col++;
    }

    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }

    vga_scroll();
    vga_update_cursor();
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_put_hex(uint32_t value) {
    char buf[16];
    utoa(value, buf, 16);
    vga_puts(buf);
}

void vga_put_dec(uint32_t value) {
    char buf[16];
    utoa(value, buf, 10);
    vga_puts(buf);
}

void vga_set_cursor(int x, int y) {
    vga_col = x;
    vga_row = y;
    vga_update_cursor();
}
