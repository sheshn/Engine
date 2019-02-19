#pragma once

#include "ツ.job.h"

#if defined(DEBUG)
    #define assert(condition) do { if (!(condition)) { *(volatile u32*)0 = 0; } } while (0)
#else
    #define assert(condition)
#endif

#define array_count(array) (sizeof((array))/sizeof((array)[0]))

#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes((value)) * 1024)
#define gigabytes(value) (megabytes((value)) * 1024)

struct Frame_Parameters
{
    Frame_Parameters* next;
    Frame_Parameters* previous;

    u64         frame_number;
    Job_Counter game_counter;
    Job_Counter render_counter;
    Job_Counter gpu_counter;
};

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