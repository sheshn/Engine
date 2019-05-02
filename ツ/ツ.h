#pragma once

#include "ツ.common.h"

struct Camera
{
    v3   position;
    quat rotation;
    m4x4 projection;
    m4x4 view;
};

struct Input_Button_State
{
    b32 is_down;
    b32 is_pressed;
};

struct Input
{
    Input_Button_State button_up;
    Input_Button_State button_down;
    Input_Button_State button_left;
    Input_Button_State button_right;

    Input_Button_State button_t;
    Input_Button_State button_space;
    Input_Button_State button_shift;

    Input_Button_State mouse_button_left;
    Input_Button_State mouse_button_right;

    v2 mouse_position;
    v3 mouse_delta;
};

struct Game_State
{
    Memory_Arena* memory_arena;
};

struct Frame_Parameters
{
    Frame_Parameters* next;
    Frame_Parameters* previous;

    u64         frame_number;
    Job_Counter game_counter;
    Job_Counter render_counter;
    Job_Counter gpu_counter;

    u64 start_time;
    u64 end_time;
    f32 delta_time;

    Input input;

    Camera camera;
    Camera DEBUG_camera;
};

void game_init(Game_State* game_state);
void game_update(Frame_Parameters* frame_params);
void game_render(Frame_Parameters* frame_params);
void game_gpu(Frame_Parameters* frame_params);