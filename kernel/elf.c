/* kernel/elf.c — ELF64 loader for x86_64 userland programs */

#include "../include/elf.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include "../include/heap.h"
#include "../include/string.h"
#include "../include/serial.h"

int elf_validate(const void* data, uint64_t size) {
    if (size < sizeof(elf64_ehdr_t)) {
        serial_puts("[ELF] File too small\n");
        return -1;
    }
    const elf64_ehdr_t* h = (const elf64_ehdr_t*)data;
    if (h->e_ident[0] != 0x7F || h->e_ident[1] != 'E' ||
        h->e_ident[2] != 'L'  || h->e_ident[3] != 'F') {
        serial_puts("[ELF] Bad magic\n");
        return -1;
    }
    if (h->e_ident[4] != ELFCLASS64)  { serial_puts("[ELF] Not ELF64\n");          return -1; }
    if (h->e_ident[5] != ELFDATA2LSB) { serial_puts("[ELF] Not little-endian\n");  return -1; }
    if (h->e_type != ET_EXEC)         { serial_puts("[ELF] Not ET_EXEC\n");        return -1; }
    if (h->e_machine != EM_X86_64)    { serial_puts("[ELF] Not x86_64\n");         return -1; }
    if (h->e_phnum && h->e_phentsize != sizeof(elf64_phdr_t)) {
        serial_puts("[ELF] Bad phentsize\n");
        return -1;
    }
    if (h->e_phoff + (uint64_t)h->e_phnum * sizeof(elf64_phdr_t) > size) {
        serial_puts("[ELF] Program headers past EOF\n");
        return -1;
    }
    return 0;
}

int elf_load(const void* data, uint64_t size, uint64_t* entry_point) {
    if (elf_validate(data, size) != 0) return -1;

    const elf64_ehdr_t* h = (const elf64_ehdr_t*)data;
    const uint8_t* file = (const uint8_t*)data;

    for (uint16_t i = 0; i < h->e_phnum; i++) {
        const elf64_phdr_t* ph = (const elf64_phdr_t*)(file + h->e_phoff + (uint64_t)i * h->e_phentsize);
        if (ph->p_type != PT_LOAD || ph->p_memsz == 0) continue;

        if (ph->p_offset + ph->p_filesz > size || ph->p_filesz > ph->p_memsz) {
            serial_puts("[ELF] Segment out of bounds\n");
            return -1;
        }

        uint64_t start = ph->p_vaddr & ~(uint64_t)(PAGE_SIZE - 1);
        uint64_t end   = (ph->p_vaddr + ph->p_memsz + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
        uint64_t flags = PAGE_WRITABLE | PAGE_USER;

        for (uint64_t va = start; va < end; va += PAGE_SIZE) {
            uint64_t frame = pmm_alloc_frame();
            if (!frame) { serial_puts("[ELF] Out of physical memory\n"); return -1; }
            vmm_map_page(frame, va, flags);
        }

        if (ph->p_filesz) memcpy((void*)(uintptr_t)ph->p_vaddr, file + ph->p_offset, ph->p_filesz);
        if (ph->p_memsz > ph->p_filesz) {
            memset((void*)(uintptr_t)(ph->p_vaddr + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);
        }
    }

    *entry_point = h->e_entry;
    serial_puts("[ELF] Entry point: ");
    serial_put_hex(h->e_entry);
    serial_puts("\n");
    return 0;
}
