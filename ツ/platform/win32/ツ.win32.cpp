#include <stdio.h>

// NOTE: Unity build
#include "../../ツ.job.cpp"

struct Frame_Parameters
{
    Frame_Parameters* next;
    Frame_Parameters* previous;

    u64         frame_number;
    Job_Counter game_counter;
    Job_Counter render_counter;
    Job_Counter gpu_counter;
};

JOB_ENTRY_POINT(game_entry_point)
{
    Frame_Parameters* frame_params = (Frame_Parameters*)data;

    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->game_counter, 0);
    }

    printf("GAME %lld\n", frame_params->frame_number);
}

JOB_ENTRY_POINT(render_entry_point)
{
    Frame_Parameters* frame_params = (Frame_Parameters*)data;

    Job game = {game_entry_point, frame_params, NULL};
    run_jobs(&game, 1, &frame_params->game_counter, JOB_PRIORITY_HIGH);
    wait_for_counter(&frame_params->game_counter, 0);

    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->render_counter, 0);
    }

    printf("RENDER %lld\n", frame_params->frame_number);
}

JOB_ENTRY_POINT(gpu_entry_point)
{
    Frame_Parameters* frame_params = (Frame_Parameters*)data;

    Job render = {render_entry_point, frame_params, NULL};
    run_jobs(&render, 1, &frame_params->render_counter, JOB_PRIORITY_HIGH);
    wait_for_counter(&frame_params->render_counter, 0);

    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->gpu_counter, 0);
    }

    printf("GPU %lld\n", frame_params->frame_number);
}

int main()
{
    printf("Hello World!\n");

    // TODO: Get system thread count
    if (!init_job_system(8))
    {
        printf("Failed to initialize job system!\n");
        return 1;
    }

    Frame_Parameters frames[16];
    for (u64 i = 0; i < 16; ++i)
    {
        frames[i].next = &frames[(i + 1) % 16];
        frames[i].previous = &frames[(i - 1 + 16) % 16];
    }
    
    Frame_Parameters* current_frame = &frames[0];
    current_frame->frame_number = 0;

    bool running = true;
    while (running)
    {
        current_frame->next->frame_number = current_frame->frame_number + 1;
        current_frame->game_counter = 1;
        current_frame->render_counter = 1;
        current_frame->gpu_counter = 1;

        Job gpu = {gpu_entry_point, current_frame, NULL};
        run_jobs(&gpu, 1, &current_frame->gpu_counter, JOB_PRIORITY_HIGH);

        current_frame = current_frame->next;

        if (current_frame->frame_number > 2)
        {
            wait_for_counter(&current_frame->previous->previous->previous->gpu_counter, 0);
        }
    }

    return 0;
}

