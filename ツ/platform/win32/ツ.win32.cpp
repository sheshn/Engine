extern "C" void* __cdecl memset(void* dest, int value, size_t count)
{
    unsigned char* bytes = (unsigned char*)dest;
    while (count--)
    {
        *bytes++ = (unsigned char)value;
    }
    return dest;
}

extern "C" void* __cdecl memcpy(void* dest, const void* src, size_t size)
{
    unsigned char* d = (unsigned char*)dest;
    unsigned char* s = (unsigned char*)src;
    while (size--)
    {
        *d++ = *s++;
    }
    return dest;
}

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdarg.h>

#include "../../ツ.platform.h"
#include "../../ツ.math.h"

// NOTE: Unity build
#include "../../ツ.job.cpp"
#include "../../ツ.renderer.cpp"
#include "../vulkan/win32/ツ.vulkan.win32.cpp"
#include "../vulkan/ツ.vulkan.cpp"
#include "../../ツ.asset.cpp"
#include "../../ツ.cpp"

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
        DEBUG_printf("Unable to read file: %s\n", filename);
    }

    CloseHandle(file_handle);
    return result;
}

void DEBUG_printf(char* format, ...)
{
    char buffer[1024];

    va_list vargs;
    va_start(vargs, format);
    wvsprintfA(buffer, format, vargs);
    va_end(vargs);

    OutputDebugString(buffer);
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
        DEBUG_printf("Failed to register window class!\n");
        return NULL;
    }

    return CreateWindowEx(0, window_class.lpszClassName, "Engine", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height, 0, 0, window_class.hInstance, NULL);
}

void process_window_messages(HWND window_handle, Frame_Parameters* frame_params)
{
    frame_params->input.mouse_delta = {};

    MSG message;
    while (PeekMessage(&message, window_handle, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u64 keycode = message.wParam;
            b32 key_was_down = (message.lParam & (1 << 30)) != 0;
            b32 key_is_down = (message.lParam & (1 << 31)) == 0;

            switch (keycode)
            {
            case VK_UP:
            case 'W':
                frame_params->input.button_up = {key_is_down, key_was_down && !key_is_down};
                break;
            case VK_LEFT:
            case 'A':
                frame_params->input.button_left = {key_is_down, key_was_down && !key_is_down};
                break;
            case VK_DOWN:
            case 'S':
                frame_params->input.button_down = {key_is_down, key_was_down && !key_is_down};
                break;
            case VK_RIGHT:
            case 'D':
                frame_params->input.button_right = {key_is_down, key_was_down && !key_is_down};
                break;
            };
        } break;
        case WM_LBUTTONDOWN:
        {
            SetCapture(window_handle);
            frame_params->input.mouse_button_left.is_down = true;
        } break;
        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            frame_params->input.mouse_button_left.is_down = false;
        } break;
        case WM_RBUTTONDOWN:
        {
            SetCapture(window_handle);
            frame_params->input.mouse_button_right.is_down = true;
        } break;
        case WM_RBUTTONUP:
        {
            ReleaseCapture();
            frame_params->input.mouse_button_right.is_down = false;
        } break;
        case WM_MOUSEWHEEL:
        {
            s32 mouse_z = (s16)(message.wParam >> 16);
            frame_params->input.mouse_delta.z = (f32)mouse_z;
        } break;
        case WM_MOUSEMOVE:
        {
            s32 mouse_x = (s32)message.lParam & 0xFFFF;
            s32 mouse_y = (s32)(message.lParam >> 16) & 0xFFFF;

            frame_params->input.mouse_position = {(f32)mouse_x, (f32)mouse_y};
            frame_params->input.mouse_delta.x = frame_params->input.mouse_position.x - frame_params->previous->input.mouse_position.x;
            frame_params->input.mouse_delta.y = frame_params->input.mouse_position.y - frame_params->previous->input.mouse_position.y;
        } break;
        default:
            TranslateMessage(&message);
            DispatchMessage(&message);
            break;
        }
    }
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

    if (frame_params->frame_number != 0)
    {
        wait_for_counter(&frame_params->previous->gpu_counter, 0);
    }

    game_gpu(frame_params);

    LARGE_INTEGER end_counter;
    QueryPerformanceCounter(&end_counter);
    frame_params->end_time = end_counter.QuadPart;
}

void __stdcall WinMainCRTStartup()
{
    u64 memory_size = gigabytes(1);
    u8* platform_memory = allocate_memory(memory_size);

    Memory_Arena platform_arena = {platform_memory, memory_size};

    HWND window_handle = create_window();
    if (!window_handle)
    {
        // TODO: Logging
        DEBUG_printf("Failed to create window!\n");
        ExitProcess(1);
    }

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    init_job_system(sys_info.dwNumberOfProcessors - 1, 4, &platform_arena);

    if (!win32_init_vulkan_renderer(window_handle, window_width, window_height))
    {
        // TODO: Logging
        DEBUG_printf("Failed to initialize Vulkan!\n");
        ExitProcess(1);
    }

    if (!init_asset_system(&platform_arena))
    {
        // TODO: Logging
        DEBUG_printf("Failed to initialize asset system!\n");
        ExitProcess(1);
    }

    Frame_Parameters frames[MAX_FRAMES] = {};
    for (u64 i = 0; i < MAX_FRAMES; ++i)
    {
        frames[i].next = &frames[(i + 1) % MAX_FRAMES];
        frames[i].previous = &frames[(i - 1 + MAX_FRAMES) % MAX_FRAMES];
    }

    Frame_Parameters* current_frame = &frames[0];
    current_frame->frame_number = 0;

    current_frame->camera.rotation = {0, 0, 0, 1};

    LARGE_INTEGER performance_frequency;
    QueryPerformanceFrequency(&performance_frequency);

    Game_State game_state = {&platform_arena};
    game_init(&game_state);

    running = true;
    while (running)
    {
        process_window_messages(window_handle, current_frame);

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

            if (current_frame->frame_number % 1000 == 0)
            {
                u64 elapsed_time = current_frame->end_time - current_frame->start_time;
                f64 ms_per_frame = (1000.0 * elapsed_time) / performance_frequency.QuadPart;
                f64 fps = performance_frequency.QuadPart / (f64)elapsed_time;
                DEBUG_printf("ms per frame: %lu, fps: %lu\n", (u32)ms_per_frame, (u32)fps);
            }
        }

        current_frame->camera = current_frame->previous->camera;
        current_frame->input = current_frame->previous->input;
    }

    ExitProcess(0);
}