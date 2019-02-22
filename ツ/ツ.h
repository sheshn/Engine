#pragma once

#include "ツ.common.h"

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
};