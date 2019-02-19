#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "../../ツ.platform.h"

// NOTE: Unity build
#include "../../ツ.job.cpp"
#include "../../ツ.renderer.cpp"
#include "../vulkan/win32/ツ.vulkan.win32.cpp"
#include "../vulkan/ツ.vulkan.cpp"
#include "../../ツ.asset.cpp"

#define MAX_FRAMES 16

internal u32 window_width = 800;
internal u32 window_height = 600;
internal b32 running = false;

u8* allocate_memory(u64 size)
{
    return (u8*)VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

b32 DEBUG_read_file(char* filename, Memory_Arena* memory_arena, u64* size, u8** data)
{
    b32 result = false;

    HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    LARGE_INTEGER file_size;
    if (file_handle != INVALID_HANDLE_VALUE && GetFileSizeEx(file_handle, &file_size))
    {
        u32 s = (u32)file_size.QuadPart;
        u8* file_data = memory_arena_reserve(memory_arena, (u64)s);

        DWORD bytes_read;
        if (ReadFile(file_handle, file_data, s, &bytes_read, 0) && bytes_read == s)
        {
            *data = file_data;
            *size = (u64)s;
            result = true;
        }
    }

    if (!result)
    {
        // TODO: Logging
        printf("Unable to read file: %s\n", filename);
    }

    CloseHandle(file_handle);
    return result;
}

LRESULT CALLBACK window_callback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_SIZE:
        window_width = LOWORD(l_param);
        window_height = HIWORD(l_param);
        renderer_resize(window_width, window_height);
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
    window_class.lpszClassName = "win32.engine";
    window_class.lpfnWndProc = window_callback;

    if (!RegisterClass(&window_class))
    {
        // TODO: Logging
        printf("Failed to register window class!\n");
        return NULL;
    }

    return CreateWindowEx(0, window_class.lpszClassName, "Engine", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height, 0, 0, window_class.hInstance, NULL);
}

JOB_ENTRY_POINT(game_entry_point)
{
    Frame_Parameters* frame_params = (Frame_Parameters*)data;

    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->game_counter, 0);
    }

    // printf("GAME %lld\n", frame_params->frame_number);
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

    // TODO: Remove this test code
    Renderer_Buffer buffer = renderer_create_buffer_reference(0);
    renderer_begin_frame(frame_params);
    renderer_draw_buffer(buffer, 64 * 4, 6);
    renderer_end_frame();
    // printf("RENDER %lld\n", frame_params->frame_number);
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

    renderer_submit_frame(frame_params);

    if (frame_params->frame_number % 110 == 0)
    {
        printf("GPU %lld\n", frame_params->frame_number);
    }
}

int main()
{
    u64 memory_size = gigabytes(1);
    u8* platform_memory = allocate_memory(memory_size);

    Memory_Arena platform_arena = {platform_memory, memory_size, 0};

    HWND window_handle = create_window();
    if (!window_handle)
    {
        // TODO: Logging
        printf("Failed to create window!\n");
        return 1;
    }

    // TODO: Get system thread count
    if (!init_job_system(8, 4))
    {
        // TODO: Logging
        printf("Failed to initialize job system!\n");
        return 1;
    }

    if (!win32_init_vulkan_renderer(window_handle, window_width, window_height))
    {
        // TODO: Logging
        printf("Failed to initialize Vulkan!\n");
        return 1;
    }

    Frame_Parameters frames[MAX_FRAMES];
    for (u64 i = 0; i < MAX_FRAMES; ++i)
    {
        frames[i].next = &frames[(i + 1) % MAX_FRAMES];
        frames[i].previous = &frames[(i - 1 + MAX_FRAMES) % MAX_FRAMES];
    }

    Frame_Parameters* current_frame = &frames[0];
    current_frame->frame_number = 0;

    running = true;
    while (running)
    {
        MSG message;
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