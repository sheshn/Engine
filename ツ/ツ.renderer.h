#pragma once

#include "../../ツ.common.h"
#include "../../ツ.math.h"

struct Vertex
{
    v4 position;
};

struct Sub_Mesh
{
    u64 index_offset;
    u64 index_count;
};

struct Mesh
{
    Vertex* vertices;
    u64     vertex_count;
    u32*    indices;
    u64     index_count;

    Sub_Mesh* sub_meshes;
    u64       sub_mesh_count;
};

void renderer_resize(u32 window_width, u32 window_height);
void renderer_submit_frame(Frame_Parameters* frame_params);

