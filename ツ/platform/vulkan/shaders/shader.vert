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
#define uv2 uvec2
#define m4x4 mat4

struct Material
{
    u32 albedo_texture_index;
    u32 normal_texture_index;
    u32 roughness_texture_index;
    u32 metallic_texture_index;
    v4  base_color;
};

struct Transform
{
    m4x4 model;
};

struct Frame_Uniforms
{
    m4x4 view_projection;
};

layout(set = 0, binding = 3) buffer transform_buffer { Transform xforms[]; };
layout(set = 1, binding = 0) uniform frame_uniform_buffer { Frame_Uniforms frame_uniforms; };

layout(location = 0) in uv2 in_instance_data;
layout(location = 1) in v4 in_position;

layout(location = 0) flat out uv2 out_instance_data;
layout(location = 1) out v2 out_uv;

out gl_PerVertex
{
    v4 gl_Position;
};

void main()
{
    Transform xform = xforms[in_instance_data.x];

    v4 position = frame_uniforms.view_projection * xform.model * v4(in_position.xyz, 1.0);
    gl_Position = v4(position.xy, 0.0, 1.0);

    out_instance_data = in_instance_data;
    out_uv = in_position.zw;
}