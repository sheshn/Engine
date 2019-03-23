#pragma once

#pragma pack(push, 1)
struct Asset
{
    u64 id;
};

struct Sub_Mesh_Info
{
    u64 data_offset;
    u32 index_offset;
    u32 index_count;
    u32 material_index;
};

struct Mesh_Info
{
    Asset asset;

    u32 sub_mesh_offset;
    u32 sub_mesh_count;
};

struct Texture_Info
{
    Asset asset;

    u64 data_offset;
    u32 width;
    u32 height;
    u32 mipmap_count;
    u32 size;
};

struct Material_Info
{
    Asset asset;

    Material material;
};
#pragma pack(pop)

void init_asset_system(Memory_Arena* memory_arena);