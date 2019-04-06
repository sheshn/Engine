#pragma once

#include "ツ.h"

void DEBUG_printf(char* format, ...);

u8* allocate_memory(u64 size);
void free_memory(void* memory, u64 size = 0);

struct Platform_File_Group
{
    u32 file_count;
};

struct Platform_File_Handle
{
    b32 has_errors;
};

Platform_File_Group* open_file_group_with_type(char* file_extension);
void close_file_group(Platform_File_Group* file_group);

Platform_File_Handle* open_next_file_in_file_group(Platform_File_Group* file_group);
void close_file_handle(Platform_File_Handle* file_handle);
void read_file(Platform_File_Handle* file_handle, u64 offset, u64 size, u8* dest);