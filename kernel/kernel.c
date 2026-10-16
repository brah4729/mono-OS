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
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/heap.h"
#include "../include/isr.h"
#include "../include/keyboard.h"
#include "../include/mouse.h"
#include "../include/ata.h"
#include "../include/framebuffer.h"
#include "../include/process.h"

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

/* ─── Memory init ────────────────────────────────────── */

static void init_memory(uint32_t mem_upper, uint64_t mbi_addr) {
    uint64_t mem_size = ((uint64_t)mem_upper + 1024) * 1024;

    /* GRUB usually puts the Multiboot2 info right after the kernel image.
     * The PMM bitmap must not overwrite it: fb_init_from_multiboot reads it later. */
    uint64_t mbi_end = mbi_addr + *(uint32_t*)(uintptr_t)mbi_addr;
    uintptr_t bitmap_addr = (uintptr_t)_kernel_end;
    if (mbi_end > bitmap_addr && mbi_end < HEAP_START) {
        bitmap_addr = (mbi_end + 15) & ~(uint64_t)15;
    }
    uint32_t* bitmap = (uint32_t*)bitmap_addr;
    pmm_init(mem_size, bitmap);

    uint64_t bitmap_size = pmm_get_block_count() / 8;
    uint64_t free_start = (bitmap_addr + bitmap_size + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
    if (free_start > HEAP_START) {
        kernel_panic("PMM bitmap overlaps the kernel heap window");
    }
    if (mem_size > free_start) {
        pmm_init_region(free_start, mem_size - free_start);
    }
    /* Heap lives at a fixed window; never hand those frames out */
    pmm_deinit_region(HEAP_START, HEAP_INITIAL_SIZE);
    /* An MBI above the bitmap (outside the heap) sits in freed frames: reserve it.
     * pmm_deinit_region does not check bits, so only touch frames freed above. */
    uint64_t mbi_page = mbi_addr & ~(uint64_t)(PAGE_SIZE - 1);
    bool mbi_in_heap = mbi_page < HEAP_START + HEAP_INITIAL_SIZE && mbi_end > HEAP_START;
    if (mbi_page >= free_start && !mbi_in_heap) {
        pmm_deinit_region(mbi_page, ((mbi_end - mbi_page) + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1));
    }

    vga_puts("  Memory: ");
    vga_put_dec(pmm_get_free_block_count() * 4);
    vga_puts(" KiB free / ");
    vga_put_dec(pmm_get_block_count() * 4);
    vga_puts(" KiB total\n");
}

/* ─── Mouse IRQ12 handler ────────────────────────────── */

static void mouse_irq12(registers_t* regs) {
    (void)regs;
    mouse_irq_handler();
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

    serial_puts("[DORI] MBI at 0x");
    serial_put_hex(mboot_info_addr);
    serial_puts("\n");
    parse_multiboot2(mboot_info_addr);

    gdt_init();
    serial_puts("[DORI] GDT initialized\n");
    pic_init();
    idt_init();
    serial_puts("[DORI] IDT + PIC initialized\n");
    pit_init(1000);
    init_memory(mb2_mem_upper, mboot_info_addr);
    vmm_init();
    heap_init();
    sti();
    selftest_memory();
    selftest_cpu();

    /* Initialize keyboard */
    keyboard_init();
    vga_puts("  [OK] PS/2 keyboard\n");

    /* Initialize mouse + register IRQ12 */
    mouse_init();
    irq_register_handler(12, mouse_irq12);
    vga_puts("  [OK] PS/2 mouse\n");
    serial_puts("[DORI] PS/2 mouse initialized\n");

    /* Initialize ATA disk driver */
    ata_init();
    if (ata_is_present()) {
        ata_drive_t drive = ata_get_drive_info();
        vga_puts("  [OK] ATA disk: ");
        vga_puts(drive.model);
        vga_puts(" (");
        vga_put_dec(drive.size_mb);
        vga_puts(" MB)\n");
    } else {
        vga_puts("  [--] No ATA disk detected\n");
    }

    fb_init_from_multiboot(mboot_info_addr);

    process_init();
    serial_puts("  [OK] Process manager\n");
    selftest_sched();

    /* Subsystem init is re-added task by task during the x86_64 port. */
    for (;;) hlt();
}
