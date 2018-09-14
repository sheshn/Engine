#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// NOTE: Unity build
#include "../../ツ.job.cpp"
#include "../vulkan/win32/ツ.vulkan.win32.cpp"
#include "../vulkan/ツ.vulkan.cpp"

global u32 window_width = 800;
global u32 window_height = 600;
global bool running = false;

LRESULT CALLBACK window_callback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_SIZE:
        window_width = LOWORD(l_param);
        window_height = HIWORD(l_param);
        break;
    case WM_CLOSE:
        running = false;
        break;
    default:
        return DefWindowProc(window, message, w_param, l_param);
        break;
    }

    return 0;
}

HWND create_window()
{
    WNDCLASS window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.hInstance = GetModuleHandle(NULL);
    window_class.lpszClassName = "Engine";
    window_class.lpfnWndProc = window_callback;

    if (!RegisterClass(&window_class))
    {
        // TODO: Logging
        printf("Failed to register window class!\n");
        return NULL;
    }

    return CreateWindowEx(0, window_class.lpszClassName, "Engine", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height, 0, 0, window_class.hInstance, NULL);
}

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
    HWND window_handle = create_window();
    if (!window_handle)
    {
        // TODO: Logging
        printf("Failed to create window!\n");
    }

    // TODO: Get system thread count
    if (!init_job_system(8))
    {
        // TODO: Logging
        printf("Failed to initialize job system!\n");
        return 1;
    }

    VkInstance vulkan_instance;
    VkSurfaceKHR vulkan_surface;
    if (!win32_init_vulkan(window_handle, &vulkan_instance, &vulkan_surface) || !init_renderer_vulkan(vulkan_instance, vulkan_surface))
    {
        // TODO: Logging
        printf("Failed to initialize Vulkan!\n");
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

    running = true;
    MSG message;

    while (running)
    {
        while (PeekMessage(&message, window_handle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

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

