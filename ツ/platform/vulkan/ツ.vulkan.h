#pragma once

#include "ツ.vulkan.generated.h"
#include "../../ツ.renderer.h"

void init_vulkan_renderer(VkInstance instance, VkSurfaceKHR surface, u32 window_width, u32 window_height, Memory_Arena* memory_arena);