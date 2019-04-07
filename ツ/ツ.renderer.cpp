#include "ツ.renderer.h"

Renderer_Buffer renderer_create_buffer_reference(u32 id)
{
    Renderer_Buffer buffer = {id};
    return buffer;
}

Renderer_Texture renderer_create_texture_reference(u32 id, u32 width, u32 height, u32 mipmap_count, Renderer_Texture_Format format)
{
    Renderer_Texture texture = {safe_cast_u32_to_u16(id), safe_cast_u32_to_u16(width), safe_cast_u32_to_u16(height), safe_cast_u32_to_u8(mipmap_count), safe_cast_u32_to_u8((u32)format)};
    return texture;
}

Renderer_Material renderer_create_material_reference(u32 id)
{
    Renderer_Material material = {id};
    return material;
}

Renderer_Transform renderer_create_transform_reference(u32 id)
{
    Renderer_Transform xform = {id};
    return xform;
}