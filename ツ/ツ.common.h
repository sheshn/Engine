#pragma once

#include "ツ.types.h"

#if defined(DEBUG)
    #define assert(condition) do { if (!(condition)) { *(volatile int*)0 = 0; } } while (0)
#else
    #define assert(condition)
#endif

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

#define memory_arena_reserve_array(arena, type, count) (type*)memory_arena_reserve((arena), sizeof(type) * (count))

