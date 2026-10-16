/* lib/string.c — Freestanding string and memory utilities */

#include "../include/string.h"

void* memset(void* dest, int val, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    while (count--) {
        *d++ = (uint8_t)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    if (d < s) {
        while (count--) *d++ = *s++;
    } else {
        d += count;
        s += count;
        while (count--) *--d = *--s;
    }
    return dest;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(uint8_t*)s1 - *(uint8_t*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(uint8_t*)s1 - *(uint8_t*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

/* Copy src into dest of total size `size`, always NUL-terminating when
 * size > 0. Returns strlen(src); a result >= size means truncation. */
size_t strlcpy(char* dest, const char* src, size_t size) {
    size_t len = strlen(src);
    if (size) {
        size_t n = (len >= size) ? size - 1 : len;
        memcpy(dest, src, n);
        dest[n] = '\0';
    }
    return len;
}

/* Append src to dest of total size `size`, always NUL-terminating when
 * dest already fits. Returns the length it tried to create. */
size_t strlcat(char* dest, const char* src, size_t size) {
    size_t dlen = 0;
    while (dlen < size && dest[dlen]) dlen++;
    if (dlen == size) return size + strlen(src);
    return dlen + strlcpy(dest + dlen, src, size - dlen);
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) return (char*)str;
        str++;
    }
    return (c == '\0') ? (char*)str : (void*)0;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    size_t nlen = strlen(needle);
    while (*haystack) {
        if (strncmp(haystack, needle, nlen) == 0) return (char*)haystack;
        haystack++;
    }
    return (void*)0;
}

void itoa(int value, char* buf, int base) {
    char* p = buf;
    char* p1, *p2;
    uint32_t ud;
    int negative = 0;

    if (base == 10 && value < 0) {
        negative = 1;
        ud = (uint32_t)(-(value + 1)) + 1;
    } else {
        ud = (uint32_t)value;
    }

    /* Divide UD by BASE until UD == 0 */
    do {
        int remainder = ud % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= base);

    if (negative) *p++ = '-';
    *p = '\0';

    /* Reverse the string */
    p1 = buf;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

void utoa(uint32_t value, char* buf, int base) {
    char* p = buf;
    char* p1, *p2;

    do {
        int remainder = value % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (value /= base);

    *p = '\0';

    /* Reverse */
    p1 = buf;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}
