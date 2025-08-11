#pragma once

// Common types, typedefs and macros that make life simpler
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

#define FOURCC(A, B, C, D) \
    ((uint32)((uint8)(A) << 0) | (uint32)((uint8)(B) << 8) | \
     (uint32)((uint8)(C) << 16) | (uint32)((uint8)(D) << 24))

#ifndef FINAL
#define Assert(expression) if(!(expression)) { *(int *)0 = 0; }
#else
#define Assert(expression)
#endif

#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)
#define Terabytes(value) (Gigabytes(value) * 1024)
