#version 450

layout(set = 0, binding = 0) uniform sampler texture_2d_sampler;
layout(set = 0, binding = 1) uniform texture2D textures_2d[2048];

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(sampler2D(textures_2d[0], texture_2d_sampler), in_uv);
}