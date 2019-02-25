#pragma once

#include "ツ.h"

u8* allocate_memory(u64 size);
b32 DEBUG_read_file(char* filename, Memory_Arena* memory_arena, u64* size, u8** data);
void DEBUG_printf(char* format, ...);