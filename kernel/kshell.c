/* kernel/kshell.c — Dori Shell (dsh) — kernel-mode command-line shell */

#include "../include/kshell.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/string.h"
#include "../include/pmm.h"
#include "../include/pit.h"
#include "../include/serial.h"
#include "../include/io.h"
#include "../include/kernel.h"

#define CMD_BUFFER_SIZE 256

static char cmd_buffer[CMD_BUFFER_SIZE];
static int  cmd_pos = 0;

static void print_prompt(void) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("dori");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("> ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
}

static void cmd_help(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("Dori Shell (dsh) — Available Commands:\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  help     — Show this help message\n");
    vga_puts("  clear    — Clear the screen\n");
    vga_puts("  meminfo  — Display memory information\n");
    vga_puts("  ver      — Show kernel version\n");
    vga_puts("  echo     — Echo text to screen\n");
    vga_puts("  uptime   — Show system uptime\n");
    vga_puts("  reboot   — Restart the system\n");
    vga_puts("  halt     — Shut down the system\n");
}

static void cmd_meminfo(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("Memory Information:\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    vga_puts("  Total blocks:  ");
    vga_put_dec(pmm_get_block_count());
    vga_puts(" (");
    vga_put_dec(pmm_get_block_count() * 4);
    vga_puts(" KiB)\n");

    vga_puts("  Used blocks:   ");
    vga_put_dec(pmm_get_used_block_count());
    vga_puts(" (");
    vga_put_dec(pmm_get_used_block_count() * 4);
    vga_puts(" KiB)\n");

    vga_puts("  Free blocks:   ");
    vga_put_dec(pmm_get_free_block_count());
    vga_puts(" (");
    vga_put_dec(pmm_get_free_block_count() * 4);
    vga_puts(" KiB)\n");
}

static void cmd_ver(void) {
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("MonoOS v" MONOOS_VERSION " — ");
    vga_puts(KERNEL_NAME);
    vga_puts(" Kernel\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Built with love and caffeine.\n");
}

static void cmd_echo(const char* args) {
    if (args && *args) {
        vga_puts(args);
    }
    vga_puts("\n");
}

static void cmd_uptime(void) {
    uint32_t ticks = pit_get_ticks();
    uint32_t seconds = ticks / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;

    vga_puts("  Uptime: ");
    vga_put_dec(hours);
    vga_puts("h ");
    vga_put_dec(minutes % 60);
    vga_puts("m ");
    vga_put_dec(seconds % 60);
    vga_puts("s (");
    vga_put_dec(ticks);
    vga_puts(" ticks)\n");
}

static void cmd_reboot(void) {
    vga_puts("Rebooting...\n");
    /* Triple fault to reboot: load empty IDT and trigger interrupt */
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
    cli();
    for (;;) hlt();
}

static void cmd_halt(void) {
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("System halted. You may turn off your computer.\n");
    cli();
    for (;;) hlt();
}

static void execute_command(const char* cmd) {
    /* Skip leading whitespace */
    while (*cmd == ' ') cmd++;

    if (*cmd == '\0') return;

    serial_puts("[DSH] > ");
    serial_puts(cmd);
    serial_puts("\n");

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
    } else if (strcmp(cmd, "meminfo") == 0) {
        cmd_meminfo();
    } else if (strcmp(cmd, "ver") == 0) {
        cmd_ver();
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        cmd_echo(cmd + 5);
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo("");
    } else if (strcmp(cmd, "uptime") == 0) {
        cmd_uptime();
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(cmd, "halt") == 0) {
        cmd_halt();
    } else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("Unknown command: ");
        vga_puts(cmd);
        vga_puts("\nType 'help' for available commands.\n");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

void kshell_init(void) {
    serial_puts("[DSH] Dori shell initialized\n");
}

void kshell_run(void) {
    print_prompt();
    cmd_pos = 0;
    memset(cmd_buffer, 0, CMD_BUFFER_SIZE);

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putchar('\n');
            cmd_buffer[cmd_pos] = '\0';
            execute_command(cmd_buffer);
            cmd_pos = 0;
            memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
            print_prompt();
        } else if (c == '\b') {
            if (cmd_pos > 0) {
                cmd_pos--;
                cmd_buffer[cmd_pos] = '\0';
                vga_putchar('\b');
            }
        } else if (c == 3) {
            /* Ctrl+C: cancel current line */
            vga_puts("^C\n");
            cmd_pos = 0;
            memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
            print_prompt();
        } else if (cmd_pos < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_pos++] = c;
            vga_putchar(c);
        }
    }
}
