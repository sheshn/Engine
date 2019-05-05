#include "ツ.h"

void game_init(Game_State* game_state)
{
    // TODO: Remove this test code
    // TODO: Handle transforms properly!
    Renderer_Transform xform_buffer = renderer_create_transform_reference(0);
    Renderer_Transfer_Operation* op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    if (op)
    {
        op->transform = xform_buffer;
        m4x4* mem = (m4x4*)op->memory;
        *mem = quat_to_m4x4(axis_angle({0, 1, 0}, radians(180.0f)) * axis_angle({1, 0, 0}, radians(90.0f)));
        renderer_queue_transfer(op);
    }

    Renderer_Transform xform_buffer2 = renderer_create_transform_reference(1);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    if (op)
    {
        op->transform = xform_buffer2;
        m4x4* mem = (m4x4*)op->memory;
        *mem = scale(identity(), 0.008);
        renderer_queue_transfer(op);
    }

    Renderer_Transform xform_buffer3 = renderer_create_transform_reference(2);
    op = renderer_request_transfer(RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    if (op)
    {
        op->transform = xform_buffer3;
        m4x4* mem = (m4x4*)op->memory;
        *mem = translate(identity(), {2, 0, 0});
        renderer_queue_transfer(op);
    }
}

b32 started_down = false;
quat camera_reference_frame = {0, 0, 0, 1};

Light lights[100] = {
    {
        LIGHT_TYPE_POINT,
        {},
        v4{0, 1, -0.5, 2},
        v4{1, 1, 1, 5}
    },
    {
        LIGHT_TYPE_POINT,
        {},
        v4{0, 4, -0.5, 3},
        v4{1, 1, 1, 50}
    }
};
u32 light_count = 3;
f32 light_velocity = 1;

void game_update(Frame_Parameters* frame_params)
{
#if 0
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

    f32 speed = 100;

    if (frame_params->input.button_up.is_down)
    {
        frame_params->camera.position += normalize(v3{0, 0, 1} * frame_params->camera.rotation) * 0.0005f * distance_to_target * speed;
    }
    else if (frame_params->input.button_down.is_down)
    {
        frame_params->camera.position += normalize(v3{0, 0, -1} * frame_params->camera.rotation) * 0.0005f * distance_to_target * speed;
    }
    else if (frame_params->input.mouse_delta.z != 0)
    {
        frame_params->camera.position += normalize(v3{0, 0, frame_params->input.mouse_delta.z} * frame_params->camera.rotation) * 0.1f * distance_to_target;
    }

    if (frame_params->input.mouse_button_left.is_down)
    {
        if (!started_down)
        {
            camera_reference_frame = frame_params->camera.rotation;
            started_down = true;
        }
        quat reference = camera_reference_frame;
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
#else
    f32 speed = 5.0f;
    f32 rotation_speed = 20.0f;
    if (frame_params->input.button_up.is_down)
    {
        frame_params->camera.position += v3{0, 0, 1} * frame_params->camera.rotation * speed * frame_params->delta_time;
    }
    if (frame_params->input.button_down.is_down)
    {
        frame_params->camera.position += v3{0, 0, -1} * frame_params->camera.rotation * speed * frame_params->delta_time;
    }
    if (frame_params->input.button_left.is_down)
    {
        frame_params->camera.position += v3{-1, 0, 0} * frame_params->camera.rotation * speed * frame_params->delta_time;
    }
    if (frame_params->input.button_right.is_down)
    {
        frame_params->camera.position += v3{1, 0, 0} * frame_params->camera.rotation * speed * frame_params->delta_time;
    }

    if (frame_params->input.mouse_button_left.is_down)
    {
        if (!started_down)
        {
            camera_reference_frame = frame_params->camera.rotation;
            started_down = true;
        }

        // TODO: Need to fix mouse_delta lag!!!
        quat reference = {0, 0, 0, 1};
        if (frame_params->input.mouse_delta.y != 0)
        {
            reference *= normalize(axis_angle(v3{1, 0, 0} * camera_reference_frame, radians(frame_params->input.mouse_delta.y * rotation_speed) * frame_params->delta_time));
        }
        if (frame_params->input.mouse_delta.x != 0)
        {
            reference *= normalize(axis_angle(v3{0, 1, 0} * camera_reference_frame, radians(frame_params->input.mouse_delta.x * rotation_speed) * frame_params->delta_time));
        }
        frame_params->camera.rotation = reference * frame_params->camera.rotation;
    }
    else
    {
        started_down = false;
    }

    if (frame_params->input.button_space.is_down)
    {
        frame_params->camera.position += v3{0, 1, 0} * speed * frame_params->delta_time;
    }
    if (frame_params->input.button_shift.is_down)
    {
        frame_params->camera.position += v3{0, -1, 0} * speed * frame_params->delta_time;
    }
#endif

    frame_params->camera.view = quat_to_m4x4(conjugate(frame_params->camera.rotation)) * translate(identity(), -1 * frame_params->camera.position);
    frame_params->camera.projection = perspective_infinite(radians(90.0f), 16.0 / 9.0f, 0.1f);

    lights[0].position.x += 1.5 * frame_params->delta_time * light_velocity;
    lights[1].position.x += 1.5 * frame_params->delta_time * light_velocity;
    if (lights[0].position.x > 11 || lights[0].position.x < -11)
    {
        light_velocity *= -1;
    }

    lights[2] = lights[0];
    lights[2].position = frame_params->camera.position;

    if (frame_params->input.button_t.is_pressed)
    {
        Light* light = lights + light_count++;
        *light = lights[0];
        light->position = frame_params->camera.position;

        frame_params->DEBUG_camera.view = translate(identity(), frame_params->camera.position) * quat_to_m4x4(frame_params->camera.rotation);
    }
}

void game_render(Frame_Parameters* frame_params)
{
    renderer_begin_frame(frame_params);

    for (u32 i = 0; i < light_count; ++i)
    {
        renderer_draw_light(lights[i]);
    }

    // draw_mesh(8, renderer_create_transform_reference(0));
    draw_mesh(128, renderer_create_transform_reference(1));
    // draw_mesh(234, renderer_create_transform_reference(2));
    renderer_end_frame();
}

void game_gpu(Frame_Parameters* frame_params)
{
    renderer_submit_frame(frame_params);
}