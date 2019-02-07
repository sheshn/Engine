#pragma once

#include "ツ.common.h"
#include "ツ.math.h"
#include "ツ.intrinsic.h"

struct Renderer_Resource_Handle
{
    u64 id;
};

struct Vertex
{
    v4 position;
};

struct Sub_Mesh
{
    u32 index_offset;
    u32 index_count;
};

struct Mesh
{
    Vertex* vertices;
    u32     vertex_count;
    u32*    indices;
    u32     index_count;

    Sub_Mesh* sub_meshes;
    u32       sub_mesh_count;
};

#define MAX_TRANSFER_OPERATIONS 256

enum Renderer_Transfer_Operation_State
{
    RENDERER_TRANSFER_OPERATION_STATE_UNLOADED,
    RENDERER_TRANSFER_OPERATION_STATE_READY,
    RENDERER_TRANSFER_OPERATION_STATE_QUEUED,
    RENDERER_TRANSFER_OPERATION_STATE_FINISHED
};

struct Renderer_Transfer_Operation
{
    Renderer_Transfer_Operation_State state;
    Renderer_Resource_Handle*         handle;

    u8* memory;
    u64 size; // NOTE: The size may be larger than what was requested
};

struct Renderer_Transfer_Queue
{
    u8* transfer_memory;
    u64 transfer_memory_size;
    u64 enqueue_location;
    u64 dequeue_location;

    Renderer_Transfer_Operation operations[MAX_TRANSFER_OPERATIONS];
    u32 operation_index;

    volatile u32 operation_count;
    volatile u64 transfer_memory_used;
};

void renderer_init_transfer_queue(Renderer_Transfer_Queue* queue, u8* memory, u64 memory_size);
Renderer_Transfer_Operation* renderer_request_transfer(Renderer_Transfer_Queue* queue, u64 transfer_size);
void renderer_queue_transfer(Renderer_Transfer_Queue* queue, Renderer_Transfer_Operation* operation);

void renderer_begin_frame(Frame_Parameters* frame_params);
void renderer_draw_buffer(Renderer_Resource_Handle buffer, u32 index_offset, u32 index_count);
void renderer_end_frame();

void renderer_submit_frame(Frame_Parameters* frame_params);
void renderer_resize(u32 window_width, u32 window_height);