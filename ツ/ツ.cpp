#include "ツ.h"

void game_init(Game_State* game_state)
{
    // TODO: Remove this test code
    Renderer_Transform xform_buffer = renderer_create_transform_reference(0);
    Renderer_Transfer_Operation* op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    if (op)
    {
        op->transform = xform_buffer;
        m4x4* mem = (m4x4*)op->memory;
        *mem = identity();
        renderer_queue_transfer(op);
    }
}

void game_update(Frame_Parameters* frame_params)
{
    if (frame_params->camera.position.x == 0 && frame_params->camera.position.y == 0 && frame_params->camera.position.z == 0)
    {
        frame_params->camera.position.z = 2;
    }

    v3 target = {0, 0, 0};
    f32 distance_to_target = length(target - frame_params->camera.position);
    if (frame_params->input.button_right.is_down)
    {
        frame_params->camera.rotation *= normalize(axis_angle(normalize(v3{0, 1, 0}), 0.001f));
    }
    else if (frame_params->input.button_left.is_down)
    {
        frame_params->camera.rotation *= normalize(axis_angle(normalize(v3{0, -1, 0}), 0.001f));
    }

    if (frame_params->input.button_up.is_down)
    {
        frame_params->camera.position += normalize(v3{0, 0, 1} * frame_params->camera.rotation) * 0.0005f * distance_to_target;
    }
    else if (frame_params->input.button_down.is_down)
    {
        frame_params->camera.position += normalize(v3{0, 0, -1} * frame_params->camera.rotation) * 0.0005f * distance_to_target;
    }
    else if (frame_params->input.mouse_delta.z != 0)
    {
        frame_params->camera.position += normalize(v3{0, 0, frame_params->input.mouse_delta.z} * frame_params->camera.rotation) * 0.1f * distance_to_target;
    }

    if (frame_params->input.mouse_button_left.is_down)
    {
        quat reference = {0, 0, 0, 1};
        if (frame_params->input.mouse_delta.y != 0)
        {
            reference *= normalize(axis_angle(normalize(v3{frame_params->input.mouse_delta.y, 0, 0}), 0.025f));
        }
        if (frame_params->input.mouse_delta.x != 0)
        {
            reference *= normalize(axis_angle(normalize(v3{0, frame_params->input.mouse_delta.x, 0}), 0.025f));
        }
        frame_params->camera.rotation *= reference;
    }

    frame_params->camera.position = normalize(v3{0, 0, -1} * frame_params->camera.rotation) * length(target - frame_params->camera.position);
    frame_params->camera.view = quat_to_m4x4(conjugate(frame_params->camera.rotation)) * translate(identity(), -1 * frame_params->camera.position);
    frame_params->camera.projection = perspective_infinite(radians(90.0f), 16.0 / 9.0f, 0.01f);
}

void game_render(Frame_Parameters* frame_params)
{
    renderer_begin_frame(frame_params);
    draw_mesh(3, renderer_create_transform_reference(0));
    renderer_end_frame();
}

void game_gpu(Frame_Parameters* frame_params)
{
    renderer_submit_frame(frame_params);
}