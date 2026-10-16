#ifndef VMM_H
#define VMM_H

#include "types.h"

#define PAGE_PRESENT    0x1UL
#define PAGE_WRITABLE   0x2UL
#define PAGE_USER       0x4UL
#define PAGE_HUGE       0x80UL
#define PAGE_ADDR_MASK  0x000FFFFFFFFFF000UL

typedef uint64_t pte_t;

typedef struct {
    pte_t entries[512];
} __attribute__((aligned(4096))) page_table_t;

void     vmm_init(void);
void     vmm_map_page(uint64_t phys, uint64_t virt, uint64_t flags);
void     vmm_unmap_page(uint64_t virt);
uint64_t vmm_get_physical(uint64_t virt);

#endif
