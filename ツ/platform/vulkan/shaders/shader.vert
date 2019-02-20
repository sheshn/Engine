#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// TODO: Add #include mechanism
#define s32 int
#define u32 uint
#define f32 float
#define f64 double

#define v2 vec2
#define v3 vec3
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

layout(set = 0, binding = 2) buffer DATA_64 { Data_64 data[]; } uniforms_64;

layout(set = 1, binding = 0) uniform PER_DRAW { Per_Draw_Uniforms uniforms[8192]; } per_draw_uniforms;

layout(location = 0) in v4 in_position;

layout(location = 0) out v2 out_uv;

out gl_PerVertex
{
    v4 gl_Position;
};

Transform data64_to_xform(Data_64 data)
{
    return Transform(
        m4x4(data.data[0],  data.data[1],  data.data[2],  data.data[3],
             data.data[4],  data.data[5],  data.data[6],  data.data[7],
             data.data[8],  data.data[9],  data.data[10], data.data[11],
             data.data[12], data.data[13], data.data[14], data.data[15])
    );
}

void main()
{
    Per_Draw_Uniforms draw_uniforms = per_draw_uniforms.uniforms[0];
    Transform xform = data64_to_xform(uniforms_64.data[draw_uniforms.xform_index]);

    v4 position = xform.model * v4(in_position.xyz, 1.0);
    gl_Position = v4(position.xy, 0.0, 1.0);

    out_uv = in_position.zw;
}