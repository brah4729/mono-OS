/* kernel/kernel.c — Dori kernel entry point */

#include "../include/kernel.h"
#include "../include/types.h"
#include "../include/vga.h"
#include "../include/serial.h"
#include "../include/io.h"
#include "../include/string.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/pic.h"
#include "../include/pit.h"
#include "../include/selftest.h"

#define MULTIBOOT2_MAGIC_CHECK 0x36D76289

/* ─── Multiboot2 tag parsing ──────────────────────────── */

#define MB2_TAG_END        0
#define MB2_TAG_BOOT_CMD   1
#define MB2_TAG_BASIC_MEM  4
#define MB2_TAG_MMAP       6
#define MB2_TAG_FRAMEBUFFER 8

typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) mb2_tag_header_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed)) mb2_tag_basic_mem_t;

/* Parsed multiboot2 info */
static uint32_t mb2_mem_upper = 0;
static uint64_t mb2_info_addr_global = 0;

static void parse_multiboot2(uint64_t mb_info_addr) {
    mb2_info_addr_global = mb_info_addr;
    uint8_t* tag_ptr = (uint8_t*)(mb_info_addr + 8);

    while (1) {
        mb2_tag_header_t* tag = (mb2_tag_header_t*)tag_ptr;
        if (tag->type == MB2_TAG_END) break;

        if (tag->type == MB2_TAG_BASIC_MEM) {
            mb2_tag_basic_mem_t* mem = (mb2_tag_basic_mem_t*)tag;
            mb2_mem_upper = mem->mem_upper;
        }

        /* Advance to next tag (8-byte aligned) */
        uint32_t tag_size = (tag->size + 7) & ~7;
        tag_ptr += tag_size;
    }
}

/* ─── Banner ──────────────────────────────────────────── */

static void print_banner(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("\n");
    vga_puts("  __  __                            ___    ____  \n");
    vga_puts(" |  \\/  |   ___    _ __     ___    / _ \\  / ___| \n");
    vga_puts(" | |\\/| |  / _ \\  | '_ \\   / _ \\  | | | | \\___ \\ \n");
    vga_puts(" | |  | | | (_) | | | | | | (_) | | |_| |  ___) |\n");
    vga_puts(" |_|  |_|  \\___/  |_| |_|  \\___/   \\___/  |____/ \n");
    vga_puts("\n");

    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("  MonoOS v" MONOOS_VERSION " - ");
    vga_puts(KERNEL_NAME);
    vga_puts(" Kernel\n");

    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  -------------------------------------\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

/* ─── Kernel panic ───────────────────────────────────── */

void kernel_panic(const char* msg) {
    cli();
    vga_set_color(VGA_WHITE, VGA_RED);
    vga_puts("\n*** DORI KERNEL PANIC ***\n");
    vga_puts(msg);
    vga_puts("\nSystem halted.\n");
    serial_puts("[PANIC] ");
    serial_puts(msg);
    serial_puts("\n");
    for (;;) hlt();
}

/* ─── Entry point ────────────────────────────────────── */

void kernel_main(uint32_t magic, uint64_t mboot_info_addr) {
    serial_init();
    serial_puts("\n[DORI] MonoOS v" MONOOS_VERSION " booting (x86_64)...\n");

    vga_init();
    print_banner();

    if (magic != MULTIBOOT2_MAGIC_CHECK) {
        kernel_panic("Not booted by a Multiboot2-compliant bootloader!");
        return;
    }
    serial_puts("[DORI] Multiboot2 verified\n");
    serial_puts("[DORI] long mode active\n");

    parse_multiboot2(mboot_info_addr);

    gdt_init();
    serial_puts("[DORI] GDT initialized\n");
    pic_init();
    idt_init();
    serial_puts("[DORI] IDT + PIC initialized\n");
    pit_init(1000);
    sti();
    selftest_cpu();

    /* Subsystem init is re-added task by task during the x86_64 port. */
    for (;;) hlt();
}
