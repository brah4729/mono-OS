/* kernel/gdt.c — x86_64 Global Descriptor Table + TSS for Dori kernel */

#include "../include/gdt.h"
#include "../include/string.h"

#define GDT_ENTRIES 7   /* null, kcode, kdata, ucode, udata, tss(lo), tss(hi) */

static uint64_t         gdt[GDT_ENTRIES];
static struct gdt_ptr   gp;
static struct tss_entry tss;

/* Code/data descriptor: base and limit are ignored in long mode except for flags */
static uint64_t make_segment(uint8_t access, uint8_t flags) {
    return ((uint64_t)0xFFFF)                    /* limit 15:0  */
         | ((uint64_t)access << 40)
         | ((uint64_t)0xF << 48)                 /* limit 19:16 */
         | ((uint64_t)(flags & 0xF0) << 48);     /* G, D/B, L   */
}

static void write_tss_descriptor(int num) {
    uint64_t base  = (uint64_t)(uintptr_t)&tss;
    uint64_t limit = sizeof(tss) - 1;

    gdt[num] = (limit & 0xFFFF)
             | ((base & 0xFFFFFF) << 16)
             | ((uint64_t)0x89 << 40)            /* present, 64-bit TSS (available) */
             | (((limit >> 16) & 0xF) << 48)
             | (((base >> 24) & 0xFF) << 56);
    gdt[num + 1] = base >> 32;
}

void gdt_init(void) {
    memset(&tss, 0, sizeof(tss));
    tss.iomap_base = sizeof(tss);

    gdt[0] = 0;
    gdt[1] = make_segment(0x9A, 0xA0);   /* kernel code: L=1, G=1 */
    gdt[2] = make_segment(0x92, 0xC0);   /* kernel data */
    gdt[3] = make_segment(0xFA, 0xA0);   /* user code, DPL3, L=1 */
    gdt[4] = make_segment(0xF2, 0xC0);   /* user data, DPL3 */
    write_tss_descriptor(5);

    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint64_t)(uintptr_t)&gdt;

    gdt_flush((uint64_t)(uintptr_t)&gp);
    tss_flush();
}

void gdt_set_kernel_stack(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}
