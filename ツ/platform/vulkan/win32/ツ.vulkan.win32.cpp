#include "ツ.vulkan.win32.h"

#define VK_FUNCTION(function) PFN_##function function;
    VK_FUNCTIONS_PLATFORM
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;

internal HMODULE      vulkan_library;
internal VkInstance   vulkan_instance;
internal VkSurfaceKHR vulkan_surface;

// TODO: Think more carefully about the renderer memory situation
internal Memory_Arena vulkan_arena;

// TODO: Provide the render resolution instead of window resolution
b32 win32_init_vulkan_renderer(HWND window, u32 window_width, u32 window_height)
{
    vulkan_library = LoadLibraryA("vulkan-1.dll");
    if (vulkan_library == NULL)
    {
        // TODO: Logging
        DEBUG_printf("Unable to load Vulkan dll!\n");
        return false;
    }

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkan_library, "vkGetInstanceProcAddr");
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");

    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "win32.engine";
    application_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    application_info.pEngineName = "win32.engine";
    application_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;

    #if defined(DEBUG)
        char* validation_layers[] = {"VK_LAYER_LUNARG_core_validation"};
        char* enabled_extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
        instance_create_info.enabledLayerCount = 1;
        instance_create_info.ppEnabledLayerNames = validation_layers;
    #else
        char* enabled_extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
    #endif

    instance_create_info.enabledExtensionCount = array_count(enabled_extensions);
    instance_create_info.ppEnabledExtensionNames = enabled_extensions;
    VkResult result = vkCreateInstance(&instance_create_info, NULL, &vulkan_instance);
    assert(result == VK_SUCCESS);

    vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(vulkan_instance, "vkCreateWin32SurfaceKHR");

    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.hwnd = window;
    surface_create_info.hinstance = GetModuleHandle(NULL);
    result = vkCreateWin32SurfaceKHR(vulkan_instance, &surface_create_info, NULL, &vulkan_surface);
    assert(result == VK_SUCCESS);

    u64 memory_size = megabytes(64);
    vulkan_arena = {(u8*)allocate_memory(memory_size), memory_size, 0};

    init_vulkan_renderer(vulkan_instance, vulkan_surface, window_width, window_height, &vulkan_arena);
    return true;
}