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

void DEBUG_printf(char* format, ...)
{
    char buffer[1024];

    va_list vargs;
    va_start(vargs, format);
    wvsprintfA(buffer, format, vargs);
    va_end(vargs);

    OutputDebugString(buffer);
}

void* allocate_memory(u64 size)
{
    return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void free_memory(void* memory, u64 size)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

struct Win32_File_Group
{
    Platform_File_Group file_group;

    HANDLE          win32_find_handle;
    WIN32_FIND_DATA win32_find_data;
};

struct Win32_File_Handle
{
    Platform_File_Handle file_handle;

    HANDLE win32_handle;
};

Platform_File_Group* open_file_group_with_type(char* file_extension)
{
    char wildcard[32] = "*.";
    for (u32 i = 2; i < sizeof(wildcard) && *file_extension; ++i, ++file_extension)
    {
        wildcard[i] = *file_extension;
    }

    WIN32_FIND_DATA find_data;
    HANDLE find_handle = FindFirstFileA(wildcard, &find_data);

    // TODO: Use memory arena?
    Win32_File_Group* file_group = (Win32_File_Group*)allocate_memory(sizeof(Win32_File_Group));

    if (find_handle != INVALID_HANDLE_VALUE)
    {
        file_group->file_group.file_count = 0;
        do
        {
            file_group->file_group.file_count++;
        }
        while (FindNextFileA(find_handle, &find_data));
        FindClose(find_handle);

        file_group->win32_find_handle = FindFirstFileA(wildcard, &file_group->win32_find_data);
    }

    return (Platform_File_Group*)file_group;
}

void close_file_group(Platform_File_Group* file_group)
{
    Win32_File_Group* group = (Win32_File_Group*)file_group;
    if (group)
    {
        if (group->win32_find_handle != INVALID_HANDLE_VALUE)
        {
            FindClose(group->win32_find_handle);
        }
        free_memory((void*)group);
    }
}

void close_file_handle(Platform_File_Handle* file_handle)
{
    Win32_File_Handle* handle = (Win32_File_Handle*)file_handle;
    if (handle)
    {
        if (handle->win32_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle->win32_handle);
        }
        free_memory((void*)handle);
    }
}

#define file_handle_push_error(handle, format, ...) do { DEBUG_printf("win32: file error: "); DEBUG_printf(format, ##__VA_ARGS__); handle->file_handle.has_errors = true; } while(0)

Platform_File_Handle* open_next_file_in_file_group(Platform_File_Group* file_group)
{
    Win32_File_Handle* file_handle = NULL;
    Win32_File_Group* group = (Win32_File_Group*)file_group;

    if (group && group->win32_find_handle != INVALID_HANDLE_VALUE)
    {
        // TODO: Use memory arena?
        file_handle = (Win32_File_Handle*)allocate_memory(sizeof(Win32_File_Handle));
        file_handle->win32_handle = CreateFileA(group->win32_find_data.cFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

        if (file_handle->win32_handle == INVALID_HANDLE_VALUE)
        {
            file_handle_push_error(file_handle, "unable to open filename: %s\n", group->win32_find_data.cFileName);
        }

        if (!FindNextFileA(group->win32_find_handle, &group->win32_find_data))
        {
            FindClose(group->win32_find_handle);
            group->win32_find_handle = INVALID_HANDLE_VALUE;
        }
    }
    return (Platform_File_Handle*)file_handle;
}

void read_data_from_file(Platform_File_Handle* file_handle, u64 offset, u64 size, void* dest)
{
    Win32_File_Handle* file = (Win32_File_Handle*)file_handle;
    if (!file || file->file_handle.has_errors)
    {
        return;
    }

    // NOTE: Windows only does reads up to 4gb!
    u32 file_size = (u32)size;

    OVERLAPPED overlapped = {};
    overlapped.Offset = (u32)((offset >> 0) & 0xFFFFFFFF);
    overlapped.OffsetHigh = (u32)((offset >> 32) & 0xFFFFFFFF);

    DWORD bytes_read;
    if (!ReadFile(file->win32_handle, dest, file_size, &bytes_read, &overlapped) || bytes_read != file_size)
    {
        file_handle_push_error(file, "unable to read file\n");
    }
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
    u8* platform_memory = (u8*)allocate_memory(memory_size);

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