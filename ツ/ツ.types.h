#pragma once

#include <stdint.h>

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;
typedef u32      b32;

#define internal static
#define external extern

#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFFUL

union v2
{
    float data[2];

    struct { float x, y; };
    struct { float u, v; };
};

union v3
{
    float data[3];

    struct { float x, y, z; };
    struct { float r, g, b; };
};

union v4
{
    float data[4];

    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
};

union quat
{
    float data[4];

    struct { float x, y, z, w; };
};

union m4x4
{
    float data[16];
    v4    columns[4];
};