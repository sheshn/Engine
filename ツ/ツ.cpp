#include "ツ.h"

void game_init(Game_State* game_state)
{
    // TODO: Remove this test code
    Memory_Arena_Marker temp_marker = memory_arena_get_marker(game_state->memory_arena);
    u8* texture_data;
    u64 texture_size = 0;
    b32 texture_result = DEBUG_read_file("../data/image1.tsu", game_state->memory_arena, &texture_size, &texture_data);
    assert(texture_result);
    Renderer_Texture renderer_texture = renderer_create_texture_reference(0, 512, 512);

    Renderer_Transfer_Operation* op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE, texture_size);
    if (op)
    {
        op->texture = renderer_texture;
        copy_memory(op->memory, texture_data, texture_size);
        renderer_queue_transfer(op);
    }

    Renderer_Buffer renderer_buffer = renderer_create_buffer_reference(0);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MESH_BUFFER, sizeof(v4) * 4 + sizeof(u32) * 6);
    if (op)
    {
        op->buffer = renderer_buffer;
        v4* mem = (v4*)op->memory;
        mem[0] = v4{-0.5, -0.5, 0, 0};
        mem[1] = v4{ 0.5, -0.5, 1, 0};
        mem[2] = v4{ 0.5,  0.5, 1, 1};
        mem[3] = v4{-0.5,  0.5, 0, 1};

        u32* index_buffer = (u32*)(op->memory + sizeof(v4) * 4);
        index_buffer[0] = 0;
        index_buffer[1] = 1;
        index_buffer[2] = 2;
        index_buffer[3] = 0;
        index_buffer[4] = 2;
        index_buffer[5] = 3;

        renderer_queue_transfer(op);
    }

    Renderer_Transform xform_buffer = renderer_create_transform_reference(0);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    if (op)
    {
        op->transform = xform_buffer;
        m4x4* mem = (m4x4*)op->memory;
        *mem = identity();
        mem->columns[3] = v4{0.1f, 0.1f, 0, 1};

        renderer_queue_transfer(op);
    }

    Renderer_Material material_buffer = renderer_create_material_reference(0);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL);
    if (op)
    {
        op->material = material_buffer;
        Material* mem = (Material*)op->memory;
        mem->albedo_texture_id = 0;
        mem->base_color_factor = v4{1.0f, 0.0f, 0.0f, 1.0f};
        renderer_queue_transfer(op);
    }

    Renderer_Transform xform_buffer2 = renderer_create_transform_reference(1);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    if (op)
    {
        op->transform = xform_buffer2;
        m4x4* mem = (m4x4*)op->memory;
        *mem = identity();
        mem->columns[3] = v4{-0.1f, -0.1f, 0, 1};

        renderer_queue_transfer(op);
    }

    Renderer_Material material_buffer2 = renderer_create_material_reference(1);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL);
    if (op)
    {
        op->material = material_buffer2;
        Material* mem = (Material*)op->memory;
        mem->albedo_texture_id = 1;
        mem->base_color_factor = v4{0.5f, 0.1f, 0.3f, 1.0f};
        renderer_queue_transfer(op);
    }

    // TODO: The validation layer will complain on the following example since transfer memory is not aligned to 16 (BC7 texel size).
    u8* texture_data2;
    u64 texture_size2 = 0;
    texture_result = DEBUG_read_file("../data/image2.tsu", game_state->memory_arena, &texture_size2, &texture_data2);
    assert(texture_result);
    Renderer_Texture renderer_texture1 = renderer_create_texture_reference(1, 512, 512);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE, texture_size2);
    if (op)
    {
        op->texture = renderer_texture1;
        copy_memory(op->memory, texture_data2, texture_size2);
        renderer_queue_transfer(op);
    }

    memory_arena_free_to_marker(game_state->memory_arena, temp_marker);
}

void game_update(Frame_Parameters* frame_params)
{
    // TODO: Remove this test code
    float amount = 0.05f;
    if (frame_params->input.button_right.is_down)
    {
        frame_params->camera.position.x += amount;
    }
    else if (frame_params->input.button_left.is_down)
    {
        frame_params->camera.position.x -= amount;
    }

    if (frame_params->input.button_up.is_down)
    {
        frame_params->camera.position.y -= amount;
    }
    else if (frame_params->input.button_down.is_down)
    {
        frame_params->camera.position.y += amount;
    }
    frame_params->camera.view = translate(identity(), frame_params->camera.position);
    frame_params->camera.projection = identity();
}

void game_render(Frame_Parameters* frame_params)
{
    // TODO: Remove this test code
    renderer_begin_frame(frame_params);
    if (frame_params->frame_number > 3)
    {
        Renderer_Buffer buffer = renderer_create_buffer_reference(0);
        Renderer_Transform xform = renderer_create_transform_reference(0);
        Renderer_Material material = renderer_create_material_reference(0);
        Renderer_Transform xform2 = renderer_create_transform_reference(1);
        Renderer_Material material2 = renderer_create_material_reference(1);

        Renderer_Material materials[] = {material, material2};
        Renderer_Transform transforms[] = {xform, xform2};
        renderer_draw_buffer(buffer, 64 * 4, 6, 2, materials, transforms);
    }
    renderer_end_frame();
}

void game_gpu(Frame_Parameters* frame_params)
{
    renderer_submit_frame(frame_params);
}