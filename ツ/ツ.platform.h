#pragma once

#include "ツ.common.h"

u8* allocate_memory(u64 size);
bool DEBUG_read_file(char* filename, Memory_Arena* memory_arena, u64* size, u8** data);