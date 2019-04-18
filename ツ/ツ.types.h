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

#define S32_MIN (1 << 31)
#define S32_MAX (~S32_MIN)

union v2
{
    float data[2];

    struct { float x, y; };
    struct { float u, v; };
};

union uv2
{
    u32 data[2];

    struct { u32 x, y; };
    struct { u32 u, v; };
};

union sv2
{
    s32 data[2];

    struct { s32 x, y; };
    struct { s32 u, v; };
};

union v3
{
    float data[3];

    struct { float x, y, z; };
    struct { float r, g, b; };
};

union uv3
{
    u32 data[3];

    struct { u32 x, y, z; };
    struct { u32 r, g, b; };
};

union sv3
{
    s32 data[3];

    struct { s32 x, y, z; };
    struct { s32 r, g, b; };
};

union v4
{
    float data[4];

    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
};

union uv4
{
    u32 data[4];

    struct { u32 x, y, z, w; };
    struct { u32 r, g, b, a; };
};

union sv4
{
    s32 data[4];

    struct { s32 x, y, z, w; };
    struct { s32 r, g, b, a; };
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