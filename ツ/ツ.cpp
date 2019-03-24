#include "ツ.h"

void game_init(Game_State* game_state)
{
    // TODO: Remove this test code
    Renderer_Transform xform_buffer = renderer_create_transform_reference(1);
    Renderer_Transfer_Operation* op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    if (op)
    {
        op->transform = xform_buffer;
        m4x4* mem = (m4x4*)op->memory;
        *mem = identity();
        mem->columns[3] = v4{0.1f, 0.1f, 0, 1};

        renderer_queue_transfer(op);
    }
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
        Renderer_Buffer buffer1 = renderer_create_buffer_reference(1);
        Renderer_Material material1 = renderer_create_material_reference(1);
        Renderer_Transform xform1 = renderer_create_transform_reference(1);
        renderer_draw_buffer(buffer1, sizeof(Vertex) * 24, 36, material1, xform1);
    }
    renderer_end_frame();
}

void game_gpu(Frame_Parameters* frame_params)
{
    renderer_submit_frame(frame_params);
}