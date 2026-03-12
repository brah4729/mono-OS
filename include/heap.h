#ifndef HEAP_H
#define HEAP_H

#include "types.h"

#define HEAP_START 0xD0000000
#define HEAP_INITIAL_SIZE 0x100000  /* 1 MiB */

void heap_init(void);
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size);
void* kmalloc_physical(size_t size, uint32_t* phys);
void kfree(void* ptr);

#endif
