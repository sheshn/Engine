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

    Asset* assets;
    u64    asset_count;
};

internal Assets assets;

b32 init_asset_system(Memory_Arena* memory_arena)
{
    assets = {memory_arena};

    Platform_File_Group* asset_file_group = open_file_group_with_type("tsu");
    if (asset_file_group->file_count > 0)
    {
        assets.files = (Asset_File*)memory_arena_reserve(memory_arena, sizeof(Asset_File) * asset_file_group->file_count);
        assets.file_count = 0;

        for (u32 i = 0; i < asset_file_group->file_count; ++i)
        {
            u32 index = assets.file_count;
            assets.files[index].handle = open_next_file_in_file_group(asset_file_group);
            read_data_from_file(assets.files[index].handle, 0, sizeof(TSU_Header), &assets.files[index].tsu_header);

            if (assets.files[index].handle->has_errors || assets.files[index].tsu_header.magic != TSU_MAGIC || assets.files[index].tsu_header.version != TSU_VERSION)
            {
                close_file_handle(assets.files[index].handle);
                DEBUG_printf("Error loading asset file (magic: %u, version: %u)!\n", assets.files[index].tsu_header.magic, assets.files[index].tsu_header.version);
                continue;
            }

            assets.asset_count += assets.files[index].tsu_header.asset_count;
            assets.file_count++;
        }
        close_file_group(asset_file_group);
    }
    else
    {
        DEBUG_printf("No asset files (.tsu) found!\n");
        return false;
    }

    // NOTE: Adding the 'null' asset here so we don't have to subtract one from the indices that each asset refers to
    assets.asset_count++;
    assets.assets = (Asset*)memory_arena_reserve(memory_arena, sizeof(Asset) * assets.asset_count);

    // TODO: Create 'null' assets

    u32 current_asset_base_index = 0;
    for (u32 index = 0; index < assets.file_count; ++index)
    {
        for (u32 i = 0; i < assets.files[index].tsu_header.asset_count; ++i)
        {
            Asset* asset = assets.assets + i + current_asset_base_index + 1;
            asset->state = ASSET_STATE_UNLOADED;
            asset->file_index = index;

            read_data_from_file(assets.files[index].handle, sizeof(TSU_Header) + i * sizeof(Asset_Info), sizeof(Asset_Info), &asset->info);

            // NOTE: The asset indices that each asset can refer to already take the 'null' asset into consideration
            switch (asset->info.type)
            {
            case ASSET_TYPE_MATERIAL:
            {
                Material_Info* material_info = &asset->info.material_info;
                material_info->material.albedo_texture_id = material_info->material.albedo_texture_id + (material_info->material.albedo_texture_id == 0 ? 0 : current_asset_base_index);
                material_info->material.normal_texture_id = material_info->material.normal_texture_id + (material_info->material.normal_texture_id == 0 ? 0 : current_asset_base_index);
                material_info->material.roughness_metallic_occlusion_texture_id = material_info->material.roughness_metallic_occlusion_texture_id + (material_info->material.roughness_metallic_occlusion_texture_id == 0 ? 0 : current_asset_base_index);
            } break;

            case ASSET_TYPE_MESH:
            {
                Mesh_Info* mesh_info = &asset->info.mesh_info;
                mesh_info->first_sub_mesh_index = mesh_info->first_sub_mesh_index + (mesh_info->first_sub_mesh_index == 0 ? 0 : current_asset_base_index);
                assert(mesh_info->first_sub_mesh_index != 0);
            } break;

            case ASSET_TYPE_SUB_MESH:
            {
                Sub_Mesh_Info* sub_mesh_info = &asset->info.sub_mesh_info;
                sub_mesh_info->material_index = sub_mesh_info->material_index + (sub_mesh_info->material_index == 0 ? 0 : current_asset_base_index);
            } break;

            default:
                break;
            }
        }
        current_asset_base_index += assets.files[index].tsu_header.asset_count;
    }

    // TODO: Remove this test code!
    Asset* mesh = &assets.assets[3];
    Asset* sub_mesh = &assets.assets[mesh->info.mesh_info.first_sub_mesh_index];
    Asset* material = &assets.assets[sub_mesh->info.sub_mesh_info.material_index];
    Asset* texture = &assets.assets[material->info.material_info.material.albedo_texture_id];

    Renderer_Buffer renderer_buffer = renderer_create_buffer_reference(1);
    Renderer_Transfer_Operation* op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MESH_BUFFER, mesh->info.data_size);
    if (op)
    {
        op->buffer = renderer_buffer;
        read_data_from_file(assets.files[mesh->file_index].handle, mesh->info.data_offset, mesh->info.data_size, op->memory);
        renderer_queue_transfer(op);
    }

    Renderer_Texture renderer_texture = renderer_create_texture_reference(1, texture->info.texture_info.width, texture->info.texture_info.height);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE, texture->info.data_size);
    if (op)
    {
        op->texture = renderer_texture;
        read_data_from_file(assets.files[texture->file_index].handle, texture->info.data_offset, texture->info.data_size, op->memory);
        renderer_queue_transfer(op);
    }

    Renderer_Material material_buffer = renderer_create_material_reference(1);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL);
    if (op)
    {
        op->material = material_buffer;
        Material* mem = (Material*)op->memory;
        *mem = material->info.material_info.material;
        mem->albedo_texture_id = renderer_texture.id;
        renderer_queue_transfer(op);
    }

    return true;
}