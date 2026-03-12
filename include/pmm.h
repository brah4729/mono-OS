#ifndef PMM_H
#define PMM_H

#include "types.h"

#define PAGE_SIZE 4096

void pmm_init(uint32_t mem_size, uint32_t* bitmap_addr);
void pmm_init_region(uint32_t base, uint32_t size);
void pmm_deinit_region(uint32_t base, uint32_t size);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t frame_addr);
uint32_t pmm_get_free_block_count(void);
uint32_t pmm_get_block_count(void);
uint32_t pmm_get_used_block_count(void);

#endif
