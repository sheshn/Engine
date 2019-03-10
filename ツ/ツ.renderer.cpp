#include "ツ.renderer.h"

Renderer_Buffer renderer_create_buffer_reference(u32 id)
{
    Renderer_Buffer buffer = {id};
    return buffer;
}
Renderer_Texture renderer_create_texture_reference(u32 id, u32 width, u32 height)
{
    Renderer_Texture texture = {id, (u16)width, (u16)height};
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