#pragma once

#include "ツ.job.h"

#if defined(DEBUG)
    #define assert(condition) do { if (!(condition)) { *(volatile u32*)0 = 0; } } while (0)
#else
    #define assert(condition)
#endif

#define invalid_code_path() assert(!"Invalid code path!")

#define array_count(array) (sizeof((array))/sizeof((array)[0]))

#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes((value)) * 1024)
#define gigabytes(value) (megabytes((value)) * 1024)

internal void copy_memory(void* dest, void* src, u64 size)
{
    u8* d = (u8*)dest;
    u8* s = (u8*)src;
    while (size--)
    {
        *d++ = *s++;
    }
}

struct Memory_Arena
{
    u8* base;
    u64 size;
    u64 used;
};

typedef u64 Memory_Arena_Marker;

internal Memory_Arena_Marker memory_arena_get_marker(Memory_Arena* arena)
{
    return arena->used;
}

internal void memory_arena_free_to_marker(Memory_Arena* arena, Memory_Arena_Marker marker)
{
    assert(marker <= arena->size);
    arena->used = marker;
}

internal u8* memory_arena_reserve(Memory_Arena* arena, u64 size)
{
    assert(arena->used + size <= arena->size);
    u8* result = arena->base + arena->used;
    arena->used += size;
    return result;
}

internal void memory_arena_reset(Memory_Arena* arena)
{
    arena->used = 0;
}

#define memory_arena_reserve_array(arena, type, count) (type*)memory_arena_reserve((arena), sizeof(type) * (count))

internal u16 safe_cast_u32_to_u16(u32 value)
{
    u16 result = (u16)value;
    assert(result == value);
    return result;
}

internal u8 safe_cast_u32_to_u8(u32 value)
{
    u8 result = (u8)value;
    assert(result == value);
    return result;
}