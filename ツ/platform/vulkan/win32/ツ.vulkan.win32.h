#pragma once

#define VKAPI_CALL __stdcall
#define VKAPI_PTR  VKAPI_CALL
#include "../ツ.vulkan.h"

#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

typedef VkFlags VkWin32SurfaceCreateFlagsKHR;

typedef struct VkWin32SurfaceCreateInfoKHR {
    VkStructureType                 sType;
    const void*                     pNext;
    VkWin32SurfaceCreateFlagsKHR    flags;
    HINSTANCE                       hinstance;
    HWND                            hwnd;
} VkWin32SurfaceCreateInfoKHR;

typedef VkResult (VKAPI_PTR *PFN_vkCreateWin32SurfaceKHR)(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);

external PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;

b32 win32_init_vulkan_renderer(HWND window, u32 window_width, u32 window_height);