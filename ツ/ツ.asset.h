#pragma once

internal u32 TSU_MAGIC = 0xE383842E; // ツ.
internal u32 TSU_VERSION = 1;

#pragma pack(push, 1)
struct TSU_Header
{
    u32 magic;
    u32 version;

    u32 texture_count;
    u32 material_count;
    u32 mesh_count;

    u64 texture_data_offset;
    u64 mesh_data_offset;

    u8 reserved[28];
};

struct Asset
{
    u64 id;
};

struct Sub_Mesh_Info
{
    u32 index_offset;
    u32 index_count;
    u32 material_index;
};

struct Mesh_Info
{
    Asset asset;

    u64 data_offset;
    u32 size;
    u32 sub_mesh_offset;
    u32 sub_mesh_count;
};

struct Texture_Info
{
    Asset asset;

    u64 data_offset;
    u32 size;
    u32 width;
    u32 height;
    u32 mipmap_count;
};

struct Material_Info
{
    Asset asset;

    Material material;
};
#pragma pack(pop)

b32 init_asset_system(Memory_Arena* memory_arena);