#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "../ツ/ツ.h"
#include "../ツ/ツ.job.cpp"
#include <stdio.h>
#include <stdlib.h>

u8* allocate_memory(u64 size)
{
    return (u8*)VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

JOB_ENTRY_POINT(game_entry_point)
{
    Frame_Parameters* frame_params = (Frame_Parameters*)data;

    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->game_counter, 0);
    }

    LARGE_INTEGER start_counter;
    QueryPerformanceCounter(&start_counter);
    frame_params->start_time = start_counter.QuadPart;

    game_update(frame_params);
}

JOB_ENTRY_POINT(render_entry_point)
{
    Frame_Parameters* frame_params = (Frame_Parameters*)data;

    Job game = {game_entry_point, frame_params, NULL};
    run_jobs(&game, 1, &frame_params->game_counter);
    wait_for_counter(&frame_params->game_counter, 0);

    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->render_counter, 0);
    }

    game_render(frame_params);
}

JOB_ENTRY_POINT(gpu_entry_point)
{
    Frame_Parameters* frame_params = (Frame_Parameters*)data;

    Job render = {render_entry_point, frame_params, NULL};
    run_jobs(&render, 1, &frame_params->render_counter);
    wait_for_counter(&frame_params->render_counter, 0);

    float f = 100.0f;
    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->gpu_counter, 0);
    }

    game_gpu(frame_params);

    LARGE_INTEGER end_counter;
    QueryPerformanceCounter(&end_counter);
    frame_params->end_time = end_counter.QuadPart;
}

void game_update(Frame_Parameters* frame_params)
{
    printf("GAME: %llu\n", frame_params->frame_number);
}

void game_render(Frame_Parameters* frame_params)
{
    printf("RENDER: %llu\n", frame_params->frame_number);
}

void game_gpu(Frame_Parameters* frame_params)
{
    printf("GPU: %llu\n", frame_params->frame_number);
}

int main()
{
    u64 memory_arena_size = megabytes(64);
    Memory_Arena memory_arena = {allocate_memory(memory_arena_size), memory_arena_size, 0};
    init_job_system(7, 4, &memory_arena);

    #define MAX_FRAMES 16
    Frame_Parameters frames[MAX_FRAMES] = {};
    for (u64 i = 0; i < MAX_FRAMES; ++i)
    {
        frames[i].next = &frames[(i + 1) % MAX_FRAMES];
        frames[i].previous = &frames[(i - 1 + MAX_FRAMES) % MAX_FRAMES];
    }

    Frame_Parameters* current_frame = &frames[0];
    current_frame->frame_number = 0;

    LARGE_INTEGER performance_frequency;
    QueryPerformanceFrequency(&performance_frequency);

    b32 running = true;
    while (running)
    {
        current_frame->next->frame_number = current_frame->frame_number + 1;
        current_frame->game_counter = 1;
        current_frame->render_counter = 1;
        current_frame->gpu_counter = 1;

        Job gpu = {gpu_entry_point, current_frame, NULL};
        run_jobs(&gpu, 1, &current_frame->gpu_counter);

        current_frame = current_frame->next;

        if (current_frame->frame_number > 2)
        {
            wait_for_counter(&current_frame->previous->previous->previous->gpu_counter, 0);
        }
    }
    return 0;
}