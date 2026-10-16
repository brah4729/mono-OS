# userland/Makefile — Build MonoOS userland programs

CROSS_PREFIX ?= $(HOME)/opt/cross/bin/x86_64-elf-
CC      = $(CROSS_PREFIX)gcc
LD      = $(CROSS_PREFIX)ld

CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
          -I ../libc/include -nostdlib -nostdinc -fno-builtin \
          -fno-stack-protector -nostartfiles -nodefaultlibs \
          -m64 -mno-red-zone -mgeneral-regs-only -fno-pic -fno-pie
LDFLAGS = -T ../libc/user.ld -nostdlib -z max-page-size=0x1000

# Library paths
CRT0    = ../libc/crt0.o
LIBC    = ../libc/libc.a

# Programs to build
PROGRAMS = hello init nix-query nix-env

.PHONY: all clean

all: $(PROGRAMS)

hello: hello.o
	$(LD) $(LDFLAGS) -o $@ $(CRT0) $< $(LIBC)
	@echo "Built: $@"

init: init.o
	$(LD) $(LDFLAGS) -o $@ $(CRT0) $< $(LIBC)
	@echo "Built: $@"

nix-query: nix-query.o
	$(LD) $(LDFLAGS) -o $@ $(CRT0) $< $(LIBC)
	@echo "Built: $@"

nix-env: nix-env.o
	$(LD) $(LDFLAGS) -o $@ $(CRT0) $< $(LIBC)
	@echo "Built: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(PROGRAMS)
	@echo "Cleaned userland."
