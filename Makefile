# MonoOS — Dori Kernel Build System
# Cross-compiler toolchain: i686-elf-gcc

# Toolchain
CROSS_PREFIX = $(HOME)/opt/cross/bin/x86_64-elf-
CC      = $(CROSS_PREFIX)gcc
AS      = nasm
LD      = $(CROSS_PREFIX)ld

# Flags
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror \
           -I include -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
           -nostartfiles -nodefaultlibs \
           -m64 -mcmodel=small -mno-red-zone -mgeneral-regs-only -fno-pic -fno-pie
ASFLAGS = -f elf64
LDFLAGS = -T linker.ld -nostdlib -z max-page-size=0x1000

# Source files
ASM_SOURCES = boot/multiboot.asm boot/gdt.asm boot/idt.asm boot/context_switch.asm
C_SOURCES   = kernel/kernel.c kernel/selftest.c kernel/gdt.c kernel/idt.c kernel/pic.c kernel/pit.c \
              kernel/pmm.c kernel/vmm.c kernel/heap.c kernel/process.c \
              drivers/vga.c drivers/serial.c drivers/keyboard.c drivers/mouse.c \
              drivers/ata.c drivers/framebuffer.c lib/string.c

# Object files
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)
C_OBJECTS   = $(C_SOURCES:.c=.o)
OBJECTS     = $(ASM_OBJECTS) $(C_OBJECTS)

# Output
KERNEL  = dori.kernel
ISO     = monoos.iso
DISKIMG = monoos-disk.img

# ─── QEMU config ───────────────────────────────────────────
# -vga std    → CRITICAL: exposes VESA-compatible adapter so
#               GRUB can set gfxmode and Multiboot2 passes the
#               framebuffer tag to the kernel. Without this,
#               fb_is_available() returns false and Oki DE
#               never starts.
QEMU_BASE = qemu-system-x86_64 \
              -m 256M \
              -vga std \
              -serial stdio \
			  -display gtk

QEMU_DISK = -drive file=$(DISKIMG),format=raw,if=ide

QEMU_BIN     = qemu-system-x86_64
BOOT_TIMEOUT = 20
# Serial-log strings that must appear for `make test-boot` to pass.
BOOT_MARKERS = "[DORI] Multiboot2 verified" "[DORI] long mode active" \
               "[TEST] utoa64 ok" "[TEST] gdt-selectors ok" "[TEST] idt-int3 ok" "[TEST] pit-ticks ok" \
               "[TEST] vmm-high-map ok" "[TEST] vmm-huge-split ok" "[TEST] heap-align ok" \
               "[DORI] PS/2 mouse initialized" "[FB] Framebuffer found" \
               "[TEST] ctx-switch ok" "[TEST] ctx-stack-align ok" "[OK] Process manager"

# ─── Targets ───────────────────────────────────────────────

.PHONY: all kernel clean run run-nodisk run-kernel debug iso disk libc userland test-boot test-no64

# Full build (kernel + libc + userland)
all: $(ISO)

# ── Kernel-only fast build (skips libc/userland) ──────────
# Use this while iterating on oki.c / framebuffer / drivers.
# Much faster: no userland compile step.
kernel: $(ISO)

$(ISO): $(KERNEL)
	@mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/$(KERNEL)
	grub-mkrescue -o $(ISO) iso/ 2>/dev/null
	@echo ""
	@echo "═══════════════════════════════════════"
	@echo "  MonoOS ISO built: $(ISO)"
	@echo "  Run with: make run"
	@echo "═══════════════════════════════════════"

$(KERNEL): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# Build monolibc (userspace C library)
libc:
	@$(MAKE) -C libc all

# Build userland programs
userland: libc
	@$(MAKE) -C userland all

# Assembly compilation
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# C compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ─── Disk image ────────────────────────────────────────────
disk: $(DISKIMG)

$(DISKIMG):
	dd if=/dev/zero of=$(DISKIMG) bs=1M count=64 2>/dev/null
	@echo "Created $(DISKIMG) (64 MB)"

# ─── QEMU run targets ──────────────────────────────────────

# Full run with disk (normal workflow)
run: $(ISO)
	$(QEMU_BASE) -cdrom $(ISO) $(QEMU_DISK)

# Run without disk (faster, use when you don't need DoriFS)
run-nodisk: $(ISO)
	$(QEMU_BASE) -cdrom $(ISO)

# Kernel-only fast build + run (use this while working on Oki DE)
# Skips libc/userland, builds kernel, launches QEMU immediately
run-kernel: kernel
	$(QEMU_BASE) -cdrom $(ISO)

# GDB debug session (attach with: gdb dori.kernel → target remote :1234)
debug: $(ISO)
	$(QEMU_BASE) -cdrom $(ISO) $(QEMU_DISK) -s -S

# Headless boot test — asserts BOOT_MARKERS appear on serial
test-boot: $(ISO)
	scripts/boot-test.sh $(QEMU_BIN) $(ISO) $(BOOT_TIMEOUT) $(BOOT_MARKERS)

# No-long-mode path — i386 QEMU CPU lacks LM, kernel must say so instead of triple-faulting
test-no64: $(ISO)
	scripts/boot-test.sh qemu-system-i386 $(ISO) 15 "[BOOT] no long mode"

# ─── Cleanup ───────────────────────────────────────────────
clean:
	rm -f $(OBJECTS) $(KERNEL) $(ISO)
	rm -f iso/boot/$(KERNEL)
	@$(MAKE) -C libc clean
	@$(MAKE) -C userland clean
	@echo "Cleaned."

cleanall: clean
	rm -f $(DISKIMG)
	@echo "Cleaned everything (including disk image)."