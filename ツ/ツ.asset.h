#pragma once

#include "ツ.renderer.h"

struct Sub_Mesh
{
    u32 index_offset;
    u32 index_count;
};

struct Mesh
{
    Vertex* vertices;
    u32     vertex_count;
    u32*    indices;
    u32     index_count;

    Sub_Mesh* sub_meshes;
    u32       sub_mesh_count;
};

struct Sub_Texture
{
    u64 offset;
    u32 size;
    u32 width;
    u32 height;
    u32 depth;
};

struct Texture
{
    u8* data;

    Sub_Texture* sub_textures;
    u32          mipmap_count;
};

void init_asset_system(Memory_Arena* memory_arena);