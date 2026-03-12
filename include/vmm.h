#ifndef VMM_H
#define VMM_H

#include "types.h"

#define PAGES_PER_TABLE   1024
#define PAGES_PER_DIR     1024
#define PAGE_PRESENT      0x1
#define PAGE_WRITABLE     0x2
#define PAGE_USER         0x4

typedef uint32_t pt_entry;
typedef uint32_t pd_entry;

typedef struct {
    pt_entry entries[PAGES_PER_TABLE];
} page_table_t;

typedef struct {
    pd_entry entries[PAGES_PER_DIR];
} page_directory_t;

void vmm_init(void);
void vmm_map_page(uint32_t phys, uint32_t virt, uint32_t flags);
void vmm_unmap_page(uint32_t virt);
uint32_t vmm_get_physical(uint32_t virt);
void vmm_switch_directory(page_directory_t* dir);

#endif
