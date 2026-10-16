/* kernel/vmm.c — Virtual Memory Manager (x86_64 4-level paging) */

#include "../include/vmm.h"
#include "../include/pmm.h"
#include "../include/string.h"
#include "../include/isr.h"
#include "../include/vga.h"
#include "../include/serial.h"
#include "../include/io.h"
#include "../include/kernel.h"

/*
 * The boot trampoline identity-maps 0–4 GiB with 2 MiB pages. We keep using
 * that PML4; page tables we allocate come from the PMM (< 256 MiB, so they
 * are reachable through the identity map).
 */
static page_table_t* pml4 = 0;

#define IDX(virt, shift) (((virt) >> (shift)) & 0x1FF)

static void page_fault_handler(registers_t* regs) {
    uint64_t addr;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(addr));

    vga_set_color(VGA_WHITE, VGA_RED);
    vga_puts("\n*** PAGE FAULT ***\n  Address: ");
    vga_put_hex(addr);
    vga_puts("\n");

    serial_puts("[PAGE FAULT] addr=");
    serial_put_hex(addr);
    serial_puts(" rip=");
    serial_put_hex(regs->rip);
    serial_puts(" err=");
    serial_put_hex(regs->err_code);
    serial_puts("\n");

    cli();
    for (;;) hlt();
}

static inline void flush_tlb_all(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile ("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

static page_table_t* alloc_table(void) {
    uint64_t phys = pmm_alloc_frame();
    if (!phys) kernel_panic("[VMM] out of frames for page tables");
    memset((void*)(uintptr_t)phys, 0, PAGE_SIZE);
    return (page_table_t*)(uintptr_t)phys;
}

/* Return the next-level table for *entry, creating it or splitting a 2 MiB page. */
static page_table_t* next_level(pte_t* entry, uint64_t flags) {
    uint64_t user = flags & PAGE_USER;

    if (!(*entry & PAGE_PRESENT)) {
        page_table_t* t = alloc_table();
        *entry = (uint64_t)(uintptr_t)t | PAGE_PRESENT | PAGE_WRITABLE | user;
        return t;
    }

    if (*entry & PAGE_HUGE) {
        uint64_t base = *entry & PAGE_ADDR_MASK & ~0x1FFFFFUL;
        uint64_t keep = *entry & (PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
        page_table_t* t = alloc_table();
        for (uint64_t i = 0; i < 512; i++) {
            t->entries[i] = (base + i * PAGE_SIZE) | keep;
        }
        *entry = (uint64_t)(uintptr_t)t | PAGE_PRESENT | PAGE_WRITABLE | user;
        flush_tlb_all();
        return t;
    }

    *entry |= user;
    return (page_table_t*)(uintptr_t)(*entry & PAGE_ADDR_MASK);
}

void vmm_map_page(uint64_t phys, uint64_t virt, uint64_t flags) {
    page_table_t* pdpt = next_level(&pml4->entries[IDX(virt, 39)], flags);
    page_table_t* pd   = next_level(&pdpt->entries[IDX(virt, 30)], flags);
    page_table_t* pt   = next_level(&pd->entries[IDX(virt, 21)], flags);
    pt->entries[IDX(virt, 12)] = (phys & PAGE_ADDR_MASK) | PAGE_PRESENT | flags;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

/* Walk without creating; returns the PTE pointer or 0. A huge PD entry returns 0. */
static pte_t* find_pte(uint64_t virt, uint64_t* huge_base) {
    *huge_base = 0;
    pte_t e = pml4->entries[IDX(virt, 39)];
    if (!(e & PAGE_PRESENT)) return 0;
    page_table_t* pdpt = (page_table_t*)(uintptr_t)(e & PAGE_ADDR_MASK);

    e = pdpt->entries[IDX(virt, 30)];
    if (!(e & PAGE_PRESENT)) return 0;
    page_table_t* pd = (page_table_t*)(uintptr_t)(e & PAGE_ADDR_MASK);

    e = pd->entries[IDX(virt, 21)];
    if (!(e & PAGE_PRESENT)) return 0;
    if (e & PAGE_HUGE) {
        *huge_base = (e & PAGE_ADDR_MASK & ~0x1FFFFFUL) | 1;   /* low bit = "is huge" */
        return 0;
    }
    page_table_t* pt = (page_table_t*)(uintptr_t)(e & PAGE_ADDR_MASK);
    return &pt->entries[IDX(virt, 12)];
}

void vmm_unmap_page(uint64_t virt) {
    uint64_t huge;
    pte_t* pte = find_pte(virt, &huge);
    if (!pte) return;                     /* unmapped, or inside a boot huge page: leave it */
    *pte = 0;
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

uint64_t vmm_get_physical(uint64_t virt) {
    uint64_t huge;
    pte_t* pte = find_pte(virt, &huge);
    if (huge) return (huge & ~1UL) + (virt & 0x1FF000UL);
    if (!pte || !(*pte & PAGE_PRESENT)) return 0;
    return *pte & PAGE_ADDR_MASK;
}

void vmm_init(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    pml4 = (page_table_t*)(uintptr_t)(cr3 & PAGE_ADDR_MASK);
    isr_register_handler(14, page_fault_handler);
    serial_puts("[VMM] 4-level paging, boot identity map 0-4 GiB adopted\n");
}
