#include "ツ.asset.h"

struct Asset_File
{
    Platform_File_Handle* handle;

    TSU_Header tsu_header;
};

enum Asset_State
{
    ASSET_STATE_UNLOADED,
    ASSET_STATE_QUEUED,
    ASSET_STATE_LOADED
};

struct Asset
{
    Asset_State state;
    Asset_Info  info;
    u32         file_index;
};

struct Assets
{
    Memory_Arena* memory_arena;

    Asset_File* files;
    u32         file_count;

    // TODO: Remove this with file streaming
    TSU_Header tsu_header;
    u8* asset_file_data;
    u64 asset_file_size;

    // TODO: Replace with Asset*
    Asset_Info* assets;
    u64         asset_count;
};

internal Assets assets;

b32 init_asset_system(Memory_Arena* memory_arena)
{
    assets = {memory_arena};

    // TODO: Remove this test code!
    Platform_File_Group* asset_file_group = open_file_group_with_type("tsu");
    if (asset_file_group->file_count > 0)
    {
        assets.files = (Asset_File*)memory_arena_reserve(memory_arena, sizeof(Asset_File) * asset_file_group->file_count);
        assets.file_count = 0;

        for (u32 i = 0; i < asset_file_group->file_count; ++i)
        {
        #if 1
            Platform_File_Handle* asset_file_handle = open_next_file_in_file_group(asset_file_group);
            {
                assets.asset_file_data = memory_arena_reserve(memory_arena, 88400);
                read_file(asset_file_handle, 0, 88400, assets.asset_file_data);
                if (asset_file_handle->has_errors)
                {
                    return false;
                }
            }
        #else
            u32 index = assets.file_count;
            assets.files[index].handle = open_next_file_in_file_group(asset_file_group);
            read_file(assets.files[index].handle, 0, sizeof(TSU_Header), (u8*)&assets.files[index].tsu_header);

            if (assets.files[index].handle->has_errors || assets.files[index].tsu_header.magic != TSU_MAGIC || assets.files[index].tsu_header.version != TSU_VERSION)
            {
                close_file_handle(assets.files[index].handle);
                DEBUG_printf("Error loading asset file (magic: %u, version: %u)!\n", assets.files[index].tsu_header.magic, assets.files[index].tsu_header.version);
                continue;
            }

            assets.asset_count += assets.files[index].tsu_header.asset_count;
            assets.file_count++;
        #endif
        }
        close_file_group(asset_file_group);
    }
    else
    {
        DEBUG_printf("No asset files (.tsu) found!\n");
        return false;
    }

    //assets.assets = (Asset_Info*)memory_arena_reserve(memory_arena, sizeof(Asset_Info) * assets.asset_count);

    // TODO: Create 'null' assets
    u32 current_asset_count = 0;
    for (u32 i = 0; i < assets.file_count; ++i)
    {
        // TODO: Rebase asset indices
    }

#if 1
    assets.tsu_header = *(TSU_Header*)assets.asset_file_data;
    if (assets.tsu_header.magic != TSU_MAGIC || assets.tsu_header.version != TSU_VERSION)
    {
        DEBUG_printf("Asset file is not a correct .tsu file (magic: %u, version: %u)!\n", assets.tsu_header.magic, assets.tsu_header.version);
        return false;
    }

    assets.assets = (Asset_Info*)(assets.asset_file_data + sizeof(TSU_Header));

    Asset_Info mesh = assets.assets[2];
    Asset_Info sub_mesh = assets.assets[mesh.mesh_info.first_sub_mesh_index];
    Asset_Info material = assets.assets[sub_mesh.sub_mesh_info.material_index == 0 ? 0 : sub_mesh.sub_mesh_info.material_index - 1];
    Asset_Info texture = assets.assets[material.material_info.material.albedo_texture_id == 0 ? 0 : material.material_info.material.albedo_texture_id - 1];

    Renderer_Buffer renderer_buffer = renderer_create_buffer_reference(1);
    Renderer_Transfer_Operation* op  = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MESH_BUFFER, mesh.data_size);
    if (op)
    {
        op->buffer = renderer_buffer;
        copy_memory(op->memory, assets.asset_file_data + mesh.data_offset, mesh.data_size);
        renderer_queue_transfer(op);
    }

    Renderer_Texture renderer_texture = renderer_create_texture_reference(1, texture.texture_info.width, texture.texture_info.height);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE, texture.data_size);
    if (op)
    {
        op->texture = renderer_texture;
        copy_memory(op->memory, assets.asset_file_data + texture.data_offset, texture.data_size);
        renderer_queue_transfer(op);
    }

    Renderer_Material material_buffer = renderer_create_material_reference(1);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL);
    if (op)
    {
        op->material = material_buffer;
        Material* mem = (Material*)op->memory;
        *mem = material.material_info.material;
        mem->albedo_texture_id = renderer_texture.id;
        renderer_queue_transfer(op);
    }
#endif

    return true;
}