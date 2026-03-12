/* kernel/pmm.c — Physical Memory Manager (bitmap allocator) */

#include "../include/pmm.h"
#include "../include/string.h"
#include "../include/serial.h"

static uint32_t* pmm_bitmap  = 0;
static uint32_t  pmm_max_blocks = 0;
static uint32_t  pmm_used_blocks = 0;

/* Set a bit in the bitmap (mark frame as used) */
static inline void bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

/* Clear a bit in the bitmap (mark frame as free) */
static inline void bitmap_clear(uint32_t bit) {
    pmm_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

/* Test a bit in the bitmap */
static inline bool bitmap_test(uint32_t bit) {
    return (pmm_bitmap[bit / 32] & (1 << (bit % 32))) != 0;
}

/* Find first free frame */
static int bitmap_first_free(void) {
    for (uint32_t i = 0; i < pmm_max_blocks / 32; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                int bit = 1 << j;
                if (!(pmm_bitmap[i] & bit)) {
                    return (int)(i * 32 + j);
                }
            }
        }
    }
    return -1;
}

void pmm_init(uint32_t mem_size, uint32_t* bitmap_addr) {
    pmm_max_blocks  = mem_size / PAGE_SIZE;
    pmm_used_blocks = pmm_max_blocks;
    pmm_bitmap      = bitmap_addr;

    /* Mark all memory as used initially */
    memset(pmm_bitmap, 0xFF, pmm_max_blocks / 8);

    serial_puts("[PMM] Initialized: ");
    char buf[16];
    utoa(pmm_max_blocks, buf, 10);
    serial_puts(buf);
    serial_puts(" blocks (");
    utoa(mem_size / 1024, buf, 10);
    serial_puts(buf);
    serial_puts(" KiB)\n");
}

void pmm_init_region(uint32_t base, uint32_t size) {
    uint32_t align  = base / PAGE_SIZE;
    uint32_t blocks = size / PAGE_SIZE;

    for (; blocks > 0; blocks--) {
        bitmap_clear(align++);
        pmm_used_blocks--;
    }

    /* First block is always set (prevent allocating address 0) */
    bitmap_set(0);
}

void pmm_deinit_region(uint32_t base, uint32_t size) {
    uint32_t align  = base / PAGE_SIZE;
    uint32_t blocks = size / PAGE_SIZE;

    for (; blocks > 0; blocks--) {
        bitmap_set(align++);
        pmm_used_blocks++;
    }
}

uint32_t pmm_alloc_frame(void) {
    if (pmm_get_free_block_count() == 0) {
        return 0; /* Out of memory */
    }

    int frame = bitmap_first_free();
    if (frame == -1) {
        return 0;
    }

    bitmap_set(frame);
    pmm_used_blocks++;

    return (uint32_t)(frame * PAGE_SIZE);
}

void pmm_free_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    bitmap_clear(frame);
    pmm_used_blocks--;
}

uint32_t pmm_get_free_block_count(void) {
    return pmm_max_blocks - pmm_used_blocks;
}

uint32_t pmm_get_block_count(void) {
    return pmm_max_blocks;
}

uint32_t pmm_get_used_block_count(void) {
    return pmm_used_blocks;
}
