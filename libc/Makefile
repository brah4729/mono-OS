# libc/Makefile — Build monolibc as a static library (libc.a)

CROSS_PREFIX ?= $(HOME)/opt/cross/bin/x86_64-elf-
CC      = $(CROSS_PREFIX)gcc
AR      = $(CROSS_PREFIX)ar
AS      = nasm

CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
          -I include -nostdlib -nostdinc -fno-builtin \
          -fno-stack-protector -nostartfiles -nodefaultlibs \
          -m64 -mno-red-zone -mgeneral-regs-only -fno-pic -fno-pie
ASFLAGS = -f elf64

# Source files
C_SOURCES = string.c stdio.c stdlib.c unistd.c
ASM_SOURCES = crt0.S

# Object files
C_OBJECTS   = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.S=.o)
OBJECTS     = $(C_OBJECTS)

# Output
LIBRARY = libc.a
CRT0    = crt0.o

.PHONY: all clean

all: $(LIBRARY) $(CRT0)

$(LIBRARY): $(OBJECTS)
	$(AR) rcs $@ $^
	@echo "Built monolibc: $(LIBRARY)"

$(CRT0): crt0.S
	$(AS) $(ASFLAGS) $< -o $@
	@echo "Built CRT startup: $(CRT0)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(CRT0) $(LIBRARY)
	@echo "Cleaned libc."
