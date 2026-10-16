#ifndef TYPES_H
#define TYPES_H

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long      uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long        int64_t;

typedef uint64_t           size_t;
typedef int64_t            ssize_t;
typedef uint64_t           uintptr_t;
typedef int64_t            intptr_t;

typedef enum { false = 0, true = 1 } bool;

#define NULL ((void*)0)

_Static_assert(sizeof(void*) == 8,     "Dori targets x86_64 (LP64)");
_Static_assert(sizeof(uint64_t) == 8,  "uint64_t must be 64-bit");
_Static_assert(sizeof(uint32_t) == 4,  "uint32_t must be 32-bit");

#endif
