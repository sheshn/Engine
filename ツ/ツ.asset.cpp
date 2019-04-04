#include "ツ.asset.h"

struct Assets
{
    Memory_Arena* memory_arena;

    TSU_Header tsu_header;

    // TODO: Remove this with file streaming
    u8* asset_file_data;
    u64 asset_file_size;

    Texture_Info*  textures;
    Material_Info* materials;
    Mesh_Info*     meshes;
};

internal Assets assets;

b32 init_asset_system(Memory_Arena* memory_arena)
{
    assets.memory_arena = memory_arena;

    // TODO: Remove this test code!
    Platform_File_Group* asset_file_group = open_file_group_with_type("tsu");
    if (asset_file_group->file_count > 0)
    {
        for (u32 i = 0; i < asset_file_group->file_count; ++i)
        {
            Platform_File_Handle* asset_file = open_next_file_in_file_group(asset_file_group);
            assets.asset_file_data = memory_arena_reserve(memory_arena, 88388);
            read_file(asset_file, 0, 88388, assets.asset_file_data);

            if (asset_file->has_errors)
            {
                return false;
            }
        }
        close_file_group(asset_file_group);
    }
    else
    {
        DEBUG_printf("No asset files (.tsu) found!\n");
        return false;
    }

    assets.tsu_header = *(TSU_Header*)assets.asset_file_data;
    if (assets.tsu_header.magic != TSU_MAGIC || assets.tsu_header.version != TSU_VERSION)
    {
        DEBUG_printf("Asset file is not a correct .tsu file (magic: %u, version: %u)!\n", assets.tsu_header.magic, assets.tsu_header.version);
        return false;
    }

    assets.textures = (Texture_Info*)(assets.asset_file_data + sizeof(TSU_Header));
    assets.materials = (Material_Info*)((u8*)assets.textures + sizeof(Texture_Info) * assets.tsu_header.texture_count);
    assets.meshes = (Mesh_Info*)((u8*)assets.materials + sizeof(Material_Info) * assets.tsu_header.material_count);

    Mesh_Info mesh = assets.meshes[1];
    Sub_Mesh_Info* sub_mesh = (Sub_Mesh_Info*)((u8*)assets.meshes + sizeof(Mesh_Info) * assets.tsu_header.mesh_count) + mesh.sub_mesh_offset;
    Material_Info* material = assets.materials + sub_mesh->material_index;
    Texture_Info* texture = assets.textures + material->material.albedo_texture_id;

    Renderer_Buffer renderer_buffer = renderer_create_buffer_reference(1);
    Renderer_Transfer_Operation* op  = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MESH_BUFFER, mesh.size);
    if (op)
    {
        op->buffer = renderer_buffer;
        copy_memory(op->memory, assets.asset_file_data + mesh.data_offset, mesh.size);
        renderer_queue_transfer(op);
    }

    Renderer_Texture renderer_texture = renderer_create_texture_reference(1, texture->width, texture->height);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE, texture->size);
    if (op)
    {
        op->texture = renderer_texture;
        copy_memory(op->memory, assets.asset_file_data + texture->data_offset, texture->size);
        renderer_queue_transfer(op);
    }

    Renderer_Material material_buffer = renderer_create_material_reference(1);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL);
    if (op)
    {
        op->material = material_buffer;
        Material* mem = (Material*)op->memory;
        *mem = material->material;
        mem->albedo_texture_id = renderer_texture.id;
        renderer_queue_transfer(op);
    }

    return true;
}