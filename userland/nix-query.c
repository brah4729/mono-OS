/* userland/nix-query.c — Query the Nix store on MonoOS
 *
 * A lightweight Nix store query tool that reads /nix/store
 * and displays installed packages, their paths, and metadata.
 *
 * Usage:
 *   nix-query            — list all packages in /nix/store
 *   nix-query -p <name>  — show info about a specific package
 *   nix-query -s <term>  — search packages by name
 */

#include "../libc/include/stdio.h"
#include "../libc/include/stdlib.h"
#include "../libc/include/unistd.h"
#include "../libc/include/string.h"

#define NIX_STORE_PATH "/nix/store"
#define MAX_PACKAGES   256
#define MAX_NAME_LEN   256

/* Package entry from /nix/store */
typedef struct {
    char name[MAX_NAME_LEN];
    char hash[33];          /* Nix store hash prefix (32 chars) */
    int  is_dir;
} nix_package_t;

/* Dirent structure matching kernel VFS */
typedef struct {
    char     name[256];
    unsigned int inode;
    unsigned char type;
} mono_dirent_t;

/* Read a mono_dirent from a directory via syscall */
static int read_dirent(const char* path, unsigned int index, mono_dirent_t* de) {
    /* SYS_READDIR = 7; x86_64 convention: rax = nr, rdi/rsi/rdx = args */
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(7L), "D"(path), "S"((long)index), "d"(de)
        : "memory"
    );
    return (int)ret;
}

/* Parse a Nix store path: /nix/store/<hash>-<name> */
static int parse_store_path(const char* dirname, char* hash, char* name) {
    /* Nix store entries look like: 0123456789abcdef01234567890abcde-package-name */
    int i = 0;

    /* Extract hash (up to 32 chars before the dash) */
    while (dirname[i] && dirname[i] != '-' && i < 32) {
        hash[i] = dirname[i];
        i++;
    }
    hash[i] = '\0';

    if (dirname[i] != '-') {
        /* Not a valid Nix store path format — use full name */
        strcpy(name, dirname);
        hash[0] = '\0';
        return -1;
    }

    /* Skip the dash */
    i++;

    /* Rest is the package name */
    strcpy(name, &dirname[i]);
    return 0;
}

/* List all packages in /nix/store */
static int cmd_list(void) {
    mono_dirent_t entry;
    unsigned int index = 0;
    int count = 0;

    printf("\033[1;36m"); /* Bold cyan */
    printf("Packages in %s:\n", NIX_STORE_PATH);
    printf("\033[0m");    /* Reset */
    printf("%-34s  %s\n", "HASH", "NAME");
    printf("%-34s  %s\n", "----", "----");

    while (read_dirent(NIX_STORE_PATH, index, &entry) == 0) {
        char hash[33] = {0};
        char name[MAX_NAME_LEN] = {0};

        if (parse_store_path(entry.name, hash, name) == 0) {
            printf("%-34s  %s\n", hash, name);
        } else {
            printf("%-34s  %s\n", "(raw)", entry.name);
        }

        count++;
        index++;
    }

    if (count == 0) {
        printf("  (store is empty)\n");
    } else {
        printf("\n%d package(s) installed.\n", count);
    }

    return 0;
}

/* Show info about a specific package */
static int cmd_info(const char* pkg_name) {
    mono_dirent_t entry;
    unsigned int index = 0;
    int found = 0;

    while (read_dirent(NIX_STORE_PATH, index, &entry) == 0) {
        char hash[33] = {0};
        char name[MAX_NAME_LEN] = {0};
        parse_store_path(entry.name, hash, name);

        if (strcmp(name, pkg_name) == 0 || strcmp(entry.name, pkg_name) == 0) {
            printf("\033[1;36mPackage: %s\033[0m\n", name);
            printf("  Store path:  %s/%s\n", NIX_STORE_PATH, entry.name);
            printf("  Hash:        %s\n", hash[0] ? hash : "(none)");
            printf("  Type:        %s\n", entry.type == 2 ? "directory" : "file");

            /* Try to read package metadata */
            char meta_path[512];
            snprintf(meta_path, sizeof(meta_path), "%s/%s/.meta", NIX_STORE_PATH, entry.name);
            int fd = open(meta_path, O_RDONLY);
            if (fd >= 0) {
                char buf[512];
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    printf("  Metadata:\n%s\n", buf);
                }
                close(fd);
            }

            found = 1;
            break;
        }
        index++;
    }

    if (!found) {
        printf("Package '%s' not found in store.\n", pkg_name);
        return 1;
    }

    return 0;
}

/* Search packages by name substring */
static int cmd_search(const char* term) {
    mono_dirent_t entry;
    unsigned int index = 0;
    int count = 0;

    printf("Searching for '%s'...\n\n", term);

    while (read_dirent(NIX_STORE_PATH, index, &entry) == 0) {
        char hash[33] = {0};
        char name[MAX_NAME_LEN] = {0};
        parse_store_path(entry.name, hash, name);

        if (strstr(name, term) || strstr(entry.name, term)) {
            printf("  %s/%s\n", NIX_STORE_PATH, entry.name);
            count++;
        }
        index++;
    }

    if (count == 0) {
        printf("  No packages matching '%s'.\n", term);
    } else {
        printf("\n%d result(s).\n", count);
    }

    return 0;
}

static void usage(void) {
    printf("nix-query — MonoOS Nix store query tool\n\n");
    printf("Usage:\n");
    printf("  nix-query          List all packages\n");
    printf("  nix-query -p NAME  Show package info\n");
    printf("  nix-query -s TERM  Search packages\n");
    printf("  nix-query -h       Show this help\n");
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        return cmd_list();
    }

    /* Simple arg parsing without getopt */
    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            usage();
            return 0;
        }
        if (strcmp(argv[1], "-p") == 0 && argc >= 3) {
            return cmd_info(argv[2]);
        }
        if (strcmp(argv[1], "-s") == 0 && argc >= 3) {
            return cmd_search(argv[2]);
        }
    }

    /* Default: treat argument as package name */
    return cmd_info(argv[1]);
}
