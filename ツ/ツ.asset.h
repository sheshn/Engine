#pragma once

internal u32 TSU_MAGIC = 0xE383842E; // ツ.
internal u32 TSU_VERSION = 1;

enum Asset_Type
{
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_MESH,
    ASSET_TYPE_SUB_MESH
};

#pragma pack(push, 1)
struct TSU_Header
{
    u32 magic;
    u32 version;
    u32 asset_count;

    u32 reserved[5];
};

struct Sub_Mesh_Info
{
    u32 index_offset;
    u32 index_count;
    u32 material_index;
};

struct Mesh_Info
{
    u32 first_sub_mesh_index;
    u32 sub_mesh_count;
};

struct Texture_Info
{
    u32 width;
    u32 height;
    u32 mipmap_count;
    u32 format;
};

struct Material_Info
{
    Material material;
};

struct Asset_Info
{
    Asset_Type type;
    u64        data_offset;
    u32        data_size;

    union
    {
        Texture_Info  texture_info;
        Material_Info material_info;
        Mesh_Info     mesh_info;
        Sub_Mesh_Info sub_mesh_info;
    };
};
#pragma pack(pop)

b32 init_asset_system(Memory_Arena* memory_arena);

void draw_mesh(u64 id, Renderer_Transform renderer_transform);