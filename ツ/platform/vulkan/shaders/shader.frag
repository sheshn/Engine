#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// TODO: Add #include mechanism
#define s32 int
#define u32 uint
#define f32 float
#define f64 double

#define v4 vec4
#define m4x4 mat4

struct Data_64
{
    f32 data[16];
};

struct Material
{
    u32 albedo_texture_index;
    u32 normal_texture_index;
    u32 roughness_texture_index;
    u32 metallic_texture_index;
    v4  base_color;

    f32 reserved[8];
};

struct Transform
{
    m4x4 model;
};

struct Per_Draw_Uniforms
{
    u32 xform_index;
    u32 material_index;
};

layout(set = 0, binding = 0) uniform sampler texture_2d_sampler;
layout(set = 0, binding = 1) uniform texture2D textures_2d[];
layout(set = 0, binding = 2) buffer DATA_64 { Data_64 data[]; } uniforms_64;

layout(set = 1, binding = 0) uniform PER_DRAW { Per_Draw_Uniforms uniforms[8192]; } per_draw_uniforms;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

Material data64_to_material(Data_64 data)
{
    return Material(floatBitsToUint(data.data[0]), floatBitsToUint(data.data[1]), floatBitsToUint(data.data[2]), floatBitsToUint(data.data[3]), v4(data.data[4], data.data[5], data.data[6], data.data[7]), f32[8](data.data[8], data.data[9], data.data[10], data.data[11], data.data[12], data.data[13], data.data[14], data.data[15]));
}

void main()
{
    Per_Draw_Uniforms draw_uniforms = per_draw_uniforms.uniforms[0];
    Material material = data64_to_material(uniforms_64.data[draw_uniforms.material_index]);
    out_color = texture(sampler2D(textures_2d[material.albedo_texture_index], texture_2d_sampler), in_uv);
}