# AGENTS.md — MonoOS

Guide for AI coding agents working in this repository. Keep it short and current; update it when architecture changes.

## What this is

MonoOS is a hobby monolithic OS for **i686 (32-bit x86)**, written in freestanding C (gnu99) and NASM.

- Kernel: **Dori** (`dori.kernel`), booted by GRUB via **Multiboot2**.
- Desktop: **Oki DE** (`kernel/oki.c`), runs inside the kernel on a VESA framebuffer.
- Fallback: text-mode kernel shell (`kernel/kshell.c`) when no framebuffer exists.
- Only tested in QEMU. Never run on real hardware with disks attached: the ATA driver writes to the first IDE disk and auto-formats it if no DoriFS is found.

## Layout

| Path | Contents |
|------|----------|
| `boot/` | `multiboot.asm` (entry `_start`), `gdt.asm`, `idt.asm` (ISR/IRQ stubs), `context_switch.asm` |
| `kernel/` | Core: `kernel.c` (init order), gdt, idt, pic, pit, pmm, vmm, heap, vfs, dorifs, syscall, process, elf, kshell, oki |
| `drivers/` | vga (text, scrolls), serial (COM1), keyboard, mouse, ata (PIO), framebuffer |
| `include/` | One header per subsystem; kernel code includes with `"../include/x.h"` |
| `lib/string.c` | Kernel string/memory helpers (`memset`, `strcpy`, `strcat`, `itoa`, `utoa`, ...) |
| `libc/` | Userspace libc (`crt0.S`, stdio, stdlib, string, unistd, `user.ld`) built to `libc.a` |
| `userland/` | User programs: `init`, `hello`, `nix-env`, `nix-query` |
| `iso/boot/grub/grub.cfg` | GRUB menu; sets `gfxmode=1024x768x32`, `gfxpayload=keep` |
| `linker.ld` | Kernel loaded at 1 MiB; exports `_kernel_end` (PMM bitmap goes right after) |
| `flake.nix` | Nix dev shell (cross toolchain, nasm, grub, xorriso, qemu) |

## Boot flow (`kernel/kernel.c: kernel_main`)

serial → VGA + banner → check Multiboot2 magic and parse tags → GDT → PIC + IDT → PIT 1000 Hz → PMM/VMM/heap → keyboard → mouse (IRQ12) → ATA → VFS → DoriFS mount at `/` (format if mount fails) → create `/nix/*` → syscalls (`int 0x80`) → processes → `sti` → `fb_init_from_multiboot` → `oki_init(); oki_run();` if a framebuffer exists, else `kshell_run()`.

## Build and run

```sh
make            # libc + userland + kernel + monoos.iso
make kernel     # kernel + ISO only (fast; use for oki/driver work)
make disk       # 64 MB monoos-disk.img (needed once for DoriFS)
make run        # QEMU with disk
make run-kernel # fast build + QEMU without disk
make debug      # QEMU -s -S; then gdb dori.kernel, target remote :1234
make clean / make cleanall
```

- Toolchain: `CROSS_PREFIX = $(HOME)/opt/cross/bin/i686-elf-` by default. Override with `make CROSS_PREFIX=i686-elf-` when needed.
- `CFLAGS` contain `-Werror -Wall -Wextra`. Any warning breaks the build. Mark unused params with `(void)x;`.
- QEMU **must** use `-vga std`. Without it there is no framebuffer and Oki does not start.
- Serial output (`serial_puts`) goes to the host terminal (`-serial stdio`). This is the main debug channel while Oki owns the screen.
- Host dev machine is Windows; build in WSL/Linux or the Nix shell.

## Conventions

- No libc in the kernel. Use `lib/string.c` and the kernel's own helpers. No floating point.
- File header comment: `/* path — short description */`. Section dividers: `/* ─── Name ─── */`.
- Log subsystem events to serial with a tag: `[DORI]`, `[OKI]`, `[SYSCALL]`.
- Register IRQ handlers with `irq_register_handler(n, fn)`. Handlers take `registers_t*`.
- Oki colors are `color_t` ARGB (`0xFFRRGGBB`); use the named `COLOR_*` palette (Catppuccin-like).
- Oki drawing goes into the back buffer; set `desktop.needs_redraw = true` instead of drawing directly to the screen.
- Do not add new commits for build outputs. `.o`, `dori.kernel`, `monoos.iso`, `monoos-disk.img` and `libc.a` are already tracked by mistake.

## Oki DE quick facts (`kernel/oki.c`, `include/oki.h`)

- `oki_run` loop: poll `keyboard_get_scancode()` → `oki_handle_key`, `keyboard_get_char()` → `oki_handle_char`; mouse arrives via callback `oki_handle_mouse`; compose at most every 33 ms; `hlt`.
- Windows: `oki_create_window/destroy/focus/move/resize`, draw with `oki_window_clear/putchar/puts/fill_rect`. Flags `OKI_WIN_DECORATED | OKI_WIN_MOVABLE | OKI_WIN_RESIZABLE`.
- Workspaces 0–3, switched with F1–F4 (`oki_switch_workspace`).
- Panel/taskbar height `OKI_PANEL_HEIGHT` = 32.
- Terminal: `g_term`, 58 cols × 18 rows, `term_execute()` handles built-ins (`help ver meminfo uptime ls dori clear`). It is separate from `kshell.c`, so commands are duplicated.

## Known state / gaps

- Scheduler: `process_schedule()` exists but the PIT IRQ does not call it, so there is no preemption.
- `SYS_FORK` returns -1 (TODO). `SYS_WAITPID` is minimal.
- Ring 3 is set up in the GDT/TSS but not fully enforced.
- `SYS_WRITE` on fd 1/2 writes to VGA text memory, which is invisible while Oki runs. The Oki terminal cannot run ELF programs yet.
- ATA is PIO only. The DoriFS bitmap cache is not write-through.
- `strcpy`/`strcat` have no bounds checks. Size the buffers carefully (for example, the terminal line is `TERM_COLS + 1`).

## Planned: x86_64 migration

The plan is in `docs/superpowers/plans/2026-10-02-x86_64-migration.md`. It has not been executed yet; the code is still i686. Read the plan before you touch boot, interrupt, memory, process, syscall or ELF code. Changes there may conflict with the migration.

Decisions locked in by the plan:
- i686 support is dropped. There is no dual-arch build.
- GRUB + Multiboot2 stay. A 32-bit trampoline in `boot/multiboot.asm` checks long mode, identity-maps 0–4 GiB with 2 MiB pages, and calls `kernel_main(rdi=magic, rsi=mbi)`.
- The kernel stays linked at 1 MiB (`-mcmodel=small`, no higher-half). The kernel image must end below `0x200000` (`HEAP_START`).
- Syscalls stay on `int 0x80` but use the Linux x86_64 registers: `rax` = number; `rdi rsi rdx r10 r8` = args; return in `rax`.
- The DoriFS on-disk layout is frozen. Static asserts check `sizeof(dorifs_superblock_t) == 4096` and `sizeof(dorifs_inode_t) == 344`.
- New verification: `make test-boot` boots QEMU headless, asserts `BOOT_MARKERS` in the serial log, and fails on `PANIC`, `PAGE FAULT` or `[TEST] ... FAIL`. Boot-time self-tests live in `kernel/selftest.c`.

Porting rules the plan applies:
- R1: `(uint32_t)ptr` → `(uintptr_t)ptr`.
- R2: `(T*)u32` → `(T*)(uintptr_t)u32`.
- R3: `char buf[16]` (or `[12]`) used with `utoa`/`itoa` → `[24]`.
- R4: control-register asm operands → `uint64_t`.
- R5: `regs->eax/ebx/...` → `rax/rdi/rsi/rdx/r10/r8`.
- R6: address-holding fields → `uintptr_t`, but never inside `dorifs_*` on-disk structs.

When a migration task lands, update the "Build and run", "Known state" and "Boot flow" sections above to match, and tick the task in the plan.

## When changing things

- New `.c` file in the kernel: add it to `C_SOURCES` in the root `Makefile`.
- New syscall: number in `include/syscall.h`, handler + table entry in `kernel/syscall.c`, wrapper in `libc/`.
- Update `README.md` and this file when features land.
