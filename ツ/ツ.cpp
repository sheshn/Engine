#include "ツ.h"

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
        frame_params->camera.position.y += amount;
    }
    else if (frame_params->input.button_down.is_down)
    {
        frame_params->camera.position.y -= amount;
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
        Renderer_Buffer xform = renderer_create_buffer_reference(0);
        Renderer_Buffer material = renderer_create_buffer_reference(1);
        Renderer_Buffer xform2 = renderer_create_buffer_reference(2);
        Renderer_Buffer material2 = renderer_create_buffer_reference(3);

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