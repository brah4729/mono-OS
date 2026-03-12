#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define COM1 0x3F8

void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char* str);
void serial_put_hex(uint32_t value);

#endif
