/* kernel/gdt.c — Global Descriptor Table setup for Dori kernel */

#include "../include/gdt.h"
#include "../include/string.h"

#define GDT_ENTRIES 6

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr   gp;
static struct tss_entry tss;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt[num].access      = access;
}

static void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = base + sizeof(tss);

    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    memset(&tss, 0, sizeof(tss));
    tss.ss0  = ss0;
    tss.esp0 = esp0;
    tss.cs   = 0x08 | 0x3;  /* Kernel code segment with RPL 3 */
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x10 | 0x3;
    tss.iomap_base = sizeof(tss);
}

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base  = (uint32_t)&gdt;

    /* Entry 0: Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Entry 1: Kernel code segment — base=0, limit=4GB, exec/read, ring 0 */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Entry 2: Kernel data segment — base=0, limit=4GB, read/write, ring 0 */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* Entry 3: User code segment — base=0, limit=4GB, exec/read, ring 3 */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* Entry 4: User data segment — base=0, limit=4GB, read/write, ring 3 */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    /* Entry 5: TSS */
    write_tss(5, 0x10, 0);

    gdt_flush((uint32_t)&gp);
    tss_flush();
}

void gdt_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}
