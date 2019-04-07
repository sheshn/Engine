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

    Job_Counter loaded_counter;
    union
    {
        Renderer_Texture  renderer_texture;
        Renderer_Material renderer_material;
        Renderer_Buffer   renderer_buffer;
    };
    Renderer_Transfer_Operation* transfer_operation;
};

struct Assets
{
    Memory_Arena* memory_arena;

    Asset_File* files;
    u32         file_count;

    Asset* assets;
    u64    asset_count;

    u32 current_renderer_texture_id;
    u32 current_renderer_material_id;
    u32 current_renderer_buffer_id;
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

    // TODO: Set asset 0 to be a 1x1 white texture? Other asset types won't care since they only use the id field
    assets.assets[0] = {ASSET_STATE_LOADED};
    assets.current_renderer_texture_id++;
    assets.current_renderer_material_id++;
    assets.current_renderer_buffer_id++;

    u32 current_asset_base_index = 0;
    for (u32 index = 0; index < assets.file_count; ++index)
    {
        for (u32 i = 0; i < assets.files[index].tsu_header.asset_count; ++i)
        {
            Asset* asset = assets.assets + i + current_asset_base_index + 1;
            *asset = {};

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
                asset->state = ASSET_STATE_LOADED;

                Sub_Mesh_Info* sub_mesh_info = &asset->info.sub_mesh_info;
                sub_mesh_info->material_index = sub_mesh_info->material_index + (sub_mesh_info->material_index == 0 ? 0 : current_asset_base_index);
            } break;

            default:
                break;
            }
        }
        current_asset_base_index += assets.files[index].tsu_header.asset_count;
    }

    return true;
}

// TODO: Manage renderer handles properly
internal Renderer_Texture get_next_renderer_texture_reference(u32 texture_width, u32 texture_height, u32 mipmap_count, u32 format)
{
    return renderer_create_texture_reference(assets.current_renderer_texture_id++, texture_width, texture_height, mipmap_count, format);
}

internal Renderer_Material get_next_renderer_material_reference()
{
    return renderer_create_material_reference(assets.current_renderer_material_id++);
}

internal Renderer_Buffer get_next_renderer_buffer_reference()
{
    return renderer_create_buffer_reference(assets.current_renderer_buffer_id++);
}

JOB_ENTRY_POINT(load_asset_entry_point)
{
    Asset* asset = (Asset*)data;

    read_data_from_file(assets.files[asset->file_index].handle, asset->info.data_offset, asset->info.data_size, asset->transfer_operation->memory);
    renderer_queue_transfer(asset->transfer_operation, &asset->loaded_counter);

    asset->transfer_operation = NULL;
}

internal Asset* get_asset(u64 id);

internal void load_texture(Asset* asset)
{
    assert(!asset->transfer_operation);

    asset->transfer_operation = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE, asset->info.data_size);
    if (asset->transfer_operation)
    {
        asset->renderer_texture = get_next_renderer_texture_reference(asset->info.texture_info.width, asset->info.texture_info.height, asset->info.texture_info.mipmap_count, asset->info.texture_info.format);
        asset->transfer_operation->texture = asset->renderer_texture;

        Job load_asset_job = {load_asset_entry_point, asset};
        run_jobs(&load_asset_job, 1, &asset->loaded_counter);
        asset->state = ASSET_STATE_QUEUED;
    }
}

internal void load_material(Asset* asset)
{
    Asset* albedo_texture = get_asset(asset->info.material_info.material.albedo_texture_id);
    Asset* normal_texture = get_asset(asset->info.material_info.material.normal_texture_id);
    Asset* roughness_metallic_occlusion_texture = get_asset(asset->info.material_info.material.roughness_metallic_occlusion_texture_id);

    if (!albedo_texture)
    {
        albedo_texture = assets.assets + asset->info.material_info.material.albedo_texture_id;
        assert(albedo_texture->info.type == ASSET_TYPE_TEXTURE);
    }
    if (!normal_texture)
    {
        normal_texture = assets.assets + asset->info.material_info.material.normal_texture_id;
        assert(normal_texture->info.type == ASSET_TYPE_TEXTURE);
    }
    if (!roughness_metallic_occlusion_texture)
    {
        roughness_metallic_occlusion_texture = assets.assets + asset->info.material_info.material.roughness_metallic_occlusion_texture_id;
        assert(roughness_metallic_occlusion_texture->info.type == ASSET_TYPE_TEXTURE);
    }

    if ((albedo_texture->state == ASSET_STATE_QUEUED || albedo_texture->state == ASSET_STATE_LOADED) &&
        (normal_texture->state == ASSET_STATE_QUEUED || normal_texture->state == ASSET_STATE_LOADED) &&
        (roughness_metallic_occlusion_texture->state == ASSET_STATE_QUEUED || roughness_metallic_occlusion_texture->state == ASSET_STATE_LOADED))
    {
        assert(!asset->transfer_operation);

        asset->transfer_operation = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL);
        if (asset->transfer_operation)
        {
            asset->renderer_material = get_next_renderer_material_reference();
            asset->transfer_operation->material = asset->renderer_material;

            Material* material = (Material*)asset->transfer_operation->memory;
            *material = asset->info.material_info.material;
            material->albedo_texture_id = albedo_texture->renderer_texture.id;
            material->normal_texture_id = normal_texture->renderer_texture.id;
            material->roughness_metallic_occlusion_texture_id = roughness_metallic_occlusion_texture->renderer_texture.id;

            renderer_queue_transfer(asset->transfer_operation, &asset->loaded_counter);

            asset->state = ASSET_STATE_QUEUED;
            asset->transfer_operation = NULL;
        }
    }
}

internal void load_mesh(Asset* asset)
{
    b32 all_materials_loaded = true;
    for (u32 i = 0; i < asset->info.mesh_info.sub_mesh_count; ++i)
    {
        Asset* sub_mesh = get_asset(asset->info.mesh_info.first_sub_mesh_index + i);
        assert(sub_mesh->state == ASSET_STATE_LOADED);

        Asset* material = get_asset(sub_mesh->info.sub_mesh_info.material_index);
        if (!material)
        {
            material = assets.assets + sub_mesh->info.sub_mesh_info.material_index;
            assert(material->info.type == ASSET_TYPE_MATERIAL);

            if (material->state == ASSET_STATE_UNLOADED)
            {
                all_materials_loaded = false;
            }
        }
    }

    if (all_materials_loaded)
    {
        assert(!asset->transfer_operation);

        asset->transfer_operation = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MESH_BUFFER, asset->info.data_size);
        if (asset->transfer_operation)
        {
            asset->renderer_buffer = get_next_renderer_buffer_reference();
            asset->transfer_operation->buffer = asset->renderer_buffer;

            Job load_asset_job = {load_asset_entry_point, asset};
            run_jobs(&load_asset_job, 1, &asset->loaded_counter);
            asset->state = ASSET_STATE_QUEUED;
        }
    }
}

internal void load_asset(Asset* asset)
{
    assert(asset->state == ASSET_STATE_UNLOADED);

    switch (asset->info.type)
    {
    case ASSET_TYPE_TEXTURE:
        load_texture(asset);
        break;
    case ASSET_TYPE_MATERIAL:
        load_material(asset);
        break;
    case ASSET_TYPE_MESH:
        load_mesh(asset);
        break;
    default:
        invalid_code_path();
        break;
    }
}

internal void resolve_asset_load_operation(Asset* asset)
{
    b32 dependencies_loaded = true;
    switch (asset->info.type)
    {
    case ASSET_TYPE_MATERIAL:
    {
        Asset* albedo_texture = get_asset(asset->info.material_info.material.albedo_texture_id);
        Asset* normal_texture = get_asset(asset->info.material_info.material.normal_texture_id);
        Asset* roughness_metallic_occlusion_texture = get_asset(asset->info.material_info.material.roughness_metallic_occlusion_texture_id);
        if (!albedo_texture || !normal_texture || !roughness_metallic_occlusion_texture)
        {
            dependencies_loaded = false;
        }
    } break;

    case ASSET_TYPE_MESH:
    {
        for (u32 i = 0; i < asset->info.mesh_info.sub_mesh_count; ++i)
        {
            Asset* sub_mesh = get_asset(asset->info.mesh_info.first_sub_mesh_index + i);
            Asset* material = get_asset(sub_mesh->info.sub_mesh_info.material_index);
            if (!sub_mesh || !material)
            {
                dependencies_loaded = false;
            }
        }
    } break;

    default:
        break;
    }

    if (dependencies_loaded)
    {
        asset->state = ASSET_STATE_LOADED;
    }
}

internal Asset* get_asset(u64 id)
{
    assert(id < assets.asset_count);

    Asset* asset = assets.assets + id;
    if (asset->state == ASSET_STATE_UNLOADED)
    {
        load_asset(asset);
    }
    else if (asset->state == ASSET_STATE_QUEUED && asset->loaded_counter == 0)
    {
        resolve_asset_load_operation(asset);
    }

    return asset->state == ASSET_STATE_LOADED ? asset : NULL;
}

void draw_mesh(u64 id, Renderer_Transform renderer_transform)
{
    Asset* mesh = get_asset(id);
    if (mesh)
    {
        assert(mesh->info.type == ASSET_TYPE_MESH);

        for (u32 i = 0; i < mesh->info.mesh_info.sub_mesh_count; ++i)
        {
            Asset* sub_mesh = get_asset(mesh->info.mesh_info.first_sub_mesh_index + i);
            Asset* material = get_asset(sub_mesh->info.sub_mesh_info.material_index);
            if (sub_mesh && material)
            {
                assert(sub_mesh->info.type == ASSET_TYPE_SUB_MESH && material->info.type == ASSET_TYPE_MATERIAL);

                renderer_draw_buffer(mesh->renderer_buffer, sub_mesh->info.sub_mesh_info.index_offset, sub_mesh->info.sub_mesh_info.index_count, material->renderer_material, renderer_transform);
            }
        }
    }
}