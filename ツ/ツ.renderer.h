#pragma once

#include "ツ.common.h"
#include "ツ.math.h"

struct Renderer_Buffer
{
    u32 id;
    u32 reserved;
};

struct Renderer_Texture
{
    u32 id;
    u16 width;
    u16 height;
};

struct Vertex
{
    v4 position;
};

#define MAX_TRANSFER_OPERATIONS 256

enum Renderer_Transfer_Operation_Type
{
    RENDERER_TRANSFER_OPERATION_TYPE_MESH_BUFFER,
    RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE
};

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
    Renderer_Transfer_Operation_Type  type;

    union
    {
        Renderer_Buffer  buffer;
        Renderer_Texture texture;
    };

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

// TODO: Consider removing 'renderer' prefix from all of these
Renderer_Buffer  renderer_create_buffer_reference(u32 id);
Renderer_Texture renderer_create_texture_reference(u32 id, u32 texture_type, u32 mipmap_count);

void renderer_init_transfer_queue(Renderer_Transfer_Queue* queue, u8* memory, u64 memory_size);
Renderer_Transfer_Operation* renderer_request_transfer(Renderer_Transfer_Queue* queue, Renderer_Transfer_Operation_Type type, u64 transfer_size);
void renderer_queue_transfer(Renderer_Transfer_Queue* queue, Renderer_Transfer_Operation* operation);

void renderer_begin_frame(Frame_Parameters* frame_params);
void renderer_draw_buffer(Renderer_Buffer buffer, u32 index_offset, u32 index_count);
void renderer_end_frame();

void renderer_submit_frame(Frame_Parameters* frame_params);
void renderer_resize(u32 window_width, u32 window_height);