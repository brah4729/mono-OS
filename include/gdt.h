#ifndef GDT_H
#define GDT_H

#include "types.h"

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18   /* use with RPL 3: 0x1B */
#define GDT_USER_DATA   0x20   /* use with RPL 3: 0x23 */
#define GDT_TSS         0x28   /* 16-byte descriptor, occupies entries 5 and 6 */

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* x86_64 Task State Segment — only RSP0 and IST are used */
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

void gdt_init(void);
void gdt_set_kernel_stack(uint64_t rsp0);

/* Defined in boot/gdt.asm */
extern void gdt_flush(uint64_t gdt_ptr);
extern void tss_flush(void);

#endif
