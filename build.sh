#!/bin/bash
set -e
export PATH="$HOME/opt/cross/bin:$PATH"
cd /mnt/c/Users/Asus/Desktop/MonoOs

LOG=/mnt/c/Users/Asus/Desktop/MonoOs/build_log.txt

echo "=== MonoOS Build Script ===" > $LOG
echo "Date: $(date)" >> $LOG
echo "" >> $LOG

echo "--- Cleaning ---" >> $LOG
rm -f boot/*.o kernel/*.o drivers/*.o lib/*.o dori.kernel monoos.iso 2>> $LOG
echo "Clean OK" >> $LOG

echo "" >> $LOG
echo "--- Assembling boot files ---" >> $LOG
nasm -f elf32 boot/multiboot.asm -o boot/multiboot.o >> $LOG 2>&1 && echo "  multiboot.asm OK" >> $LOG || echo "  multiboot.asm FAILED" >> $LOG
nasm -f elf32 boot/gdt.asm -o boot/gdt.o >> $LOG 2>&1 && echo "  gdt.asm OK" >> $LOG || echo "  gdt.asm FAILED" >> $LOG
nasm -f elf32 boot/idt.asm -o boot/idt.o >> $LOG 2>&1 && echo "  idt.asm OK" >> $LOG || echo "  idt.asm FAILED" >> $LOG

echo "" >> $LOG
echo "--- Compiling C files ---" >> $LOG
CFLAGS="-std=gnu99 -ffreestanding -O2 -Wall -Wextra -Werror -I include -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs"

for f in kernel/kernel.c kernel/gdt.c kernel/idt.c kernel/pic.c kernel/pit.c kernel/pmm.c kernel/vmm.c kernel/heap.c kernel/kshell.c drivers/vga.c drivers/keyboard.c drivers/serial.c lib/string.c; do
    out="${f%.c}.o"
    i686-elf-gcc $CFLAGS -c "$f" -o "$out" >> $LOG 2>&1 && echo "  $f OK" >> $LOG || echo "  $f FAILED" >> $LOG
done

echo "" >> $LOG
echo "--- Linking ---" >> $LOG
i686-elf-ld -T linker.ld -nostdlib -o dori.kernel \
    boot/multiboot.o boot/gdt.o boot/idt.o \
    kernel/kernel.o kernel/gdt.o kernel/idt.o kernel/pic.o kernel/pit.o \
    kernel/pmm.o kernel/vmm.o kernel/heap.o kernel/kshell.o \
    drivers/vga.o drivers/keyboard.o drivers/serial.o \
    lib/string.o >> $LOG 2>&1 && echo "  Link OK" >> $LOG || echo "  Link FAILED" >> $LOG

echo "" >> $LOG
echo "--- ISO Generation ---" >> $LOG
mkdir -p iso/boot/grub
cp dori.kernel iso/boot/dori.kernel
grub-mkrescue -o monoos.iso iso/ >> $LOG 2>&1 && echo "  ISO OK" >> $LOG || echo "  ISO FAILED" >> $LOG

echo "" >> $LOG
ls -la dori.kernel monoos.iso >> $LOG 2>&1
echo "" >> $LOG
echo "=== BUILD COMPLETE ===" >> $LOG
