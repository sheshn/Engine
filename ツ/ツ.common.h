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

internal u8* memory_arena_reserve(Memory_Arena* arena, u64 size)
{
    assert(arena->used + size <= arena->size);
    u8* result = arena->base + arena->used;
    arena->used += size;
    return result;
}

#define memory_arena_reserve_array(arena, type, count) (type*)memory_arena_reserve((arena), sizeof(type) * (count))

