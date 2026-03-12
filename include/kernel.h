#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

#define MONOOS_VERSION "0.1"
#define KERNEL_NAME    "Dori"

/* Multiboot info structure (simplified) */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

/* Multiboot memory map entry */
typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

#define MULTIBOOT_MEMORY_AVAILABLE 1

void kernel_panic(const char* msg);

/* Symbols from linker script */
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

#endif
