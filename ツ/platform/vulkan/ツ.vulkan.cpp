#include "ツ.vulkan.h"

#define VK_FUNCTION(function) PFN_##function function;
    VK_FUNCTIONS_DEBUG
    VK_FUNCTIONS_INSTANCE
    VK_FUNCTIONS_DEVICE
#undef VK_FUNCTION

internal VkDebugUtilsMessengerEXT debug_messenger;

internal VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    // TODO: Logging
    // TODO: Better log message
    printf("Vulkan: %s\n", callback_data->pMessage);
    return false;
}

#define MAX_SWAPCHAIN_IMAGES 3
#define MAX_FRAME_RESOURCES  3
#define STAGING_BUFFER_SIZE (64 * 1024 * 1024)
#define MESH_BUFFER_SIZE    (256 * 1024 * 1024)

struct Vulkan_Context
{
    VkInstance                       instance;
    VkDevice                         device;
    VkPhysicalDevice                 physical_device;
    VkPhysicalDeviceProperties       physical_device_properties;
    VkPhysicalDeviceFeatures         physical_device_features;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;

    VkQueue graphics_queue;
    u32     graphics_queue_index;

    VkQueue present_queue;
    u32     present_queue_index;

    VkQueue transfer_queue;
    u32     transfer_queue_index;

    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR surface_format;
    VkSwapchainKHR     swapchain;
    VkExtent2D         swapchain_extent;
    VkImage            swapchain_images[MAX_SWAPCHAIN_IMAGES];
    VkImageView        swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
    u32                swapchain_image_count;

    // TODO: Improve renderer memory situation
    Memory_Arena* memory_arena;
};

internal Vulkan_Context vulkan_context;

struct Vulkan_Memory_Block
{
    VkDeviceMemory device_memory;
    VkMemoryType   memory_type;
    u32            memory_type_index;

    u8* base;
    u64 size;
    u64 used;
};

internal u32 get_memory_type_index(u32 memory_type_bits, VkMemoryPropertyFlags memory_properties)
{
    u32 memory_type_index = 0;
    for (u32 i = 0; i < vulkan_context.physical_device_memory_properties.memoryTypeCount; ++i)
    {
        if ((memory_type_bits & (1 << i)) && (vulkan_context.physical_device_memory_properties.memoryTypes[i].propertyFlags & memory_properties) == memory_properties)
        {
            memory_type_index = i;
            break;
        }
    }
    return memory_type_index;
}

internal bool allocate_vulkan_memory_block(u64 size, u32 memory_type_index, Vulkan_Memory_Block* memory_block)
{
    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = size;
    allocate_info.memoryTypeIndex = memory_type_index;
    if (vkAllocateMemory(vulkan_context.device, &allocate_info, NULL, &memory_block->device_memory) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to allocate Vulkan memory block!\n");
        return false;
    }

    memory_block->memory_type = vulkan_context.physical_device_memory_properties.memoryTypes[memory_type_index];
    memory_block->memory_type_index = memory_type_index;
    memory_block->size = size;
    memory_block->used = 0;

    if (memory_block->memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(vulkan_context.device, memory_block->device_memory, 0, VK_WHOLE_SIZE, 0, (void**)&memory_block->base);
    }
    return true;
}

internal bool vulkan_memory_block_reserve_buffer(Vulkan_Memory_Block* memory_block, VkBufferCreateInfo* buffer_create_info, VkMemoryPropertyFlags memory_properties, VkBuffer* buffer)
{
    if (vkCreateBuffer(vulkan_context.device, buffer_create_info, NULL, buffer) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan buffer!\n");
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vulkan_context.device, *buffer, &memory_requirements);

    // TODO: Take memory_requirements alignment into account
    assert((memory_block->used + memory_requirements.size <= memory_block->size) && (memory_properties & memory_block->memory_type.propertyFlags) == memory_properties && (memory_requirements.memoryTypeBits & (1 << memory_block->memory_type_index)));

    if (vkBindBufferMemory(vulkan_context.device, *buffer, memory_block->device_memory, memory_block->used) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to bind Vulkan buffer to device memory!\n");
        return false;
    }

    memory_block->used += memory_requirements.size;
    return true;
}

internal bool allocate_vulkan_buffer(VkBufferCreateInfo* buffer_create_info, VkMemoryPropertyFlags memory_properties, VkBuffer* buffer, Vulkan_Memory_Block* memory_block)
{
    if (vkCreateBuffer(vulkan_context.device, buffer_create_info, NULL, buffer) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan buffer!\n");
        return false;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vulkan_context.device, *buffer, &memory_requirements);

    if (!allocate_vulkan_memory_block(memory_requirements.size, get_memory_type_index(memory_requirements.memoryTypeBits, memory_properties), memory_block))
    {
        return false;
    }

    if (vkBindBufferMemory(vulkan_context.device, *buffer, memory_block->device_memory, 0) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to bind Vulkan buffer to device memory!\n");
        return false;
    }

    memory_block->used += memory_requirements.size;
    return true;
}

struct Frame_Resources
{
    VkCommandBuffer command_buffer;
    VkSemaphore     image_available_semaphore;
    VkSemaphore     render_finished_semaphore;
    u32             swapchain_image_index;
};

struct Renderer
{
    Frame_Resources frame_resources[MAX_FRAME_RESOURCES];

    Vulkan_Memory_Block staging_memory_block;
    Memory_Arena        staging_arena;
    VkBuffer            staging_buffer;

    Vulkan_Memory_Block mesh_memory_block;
    VkBuffer            mesh_buffer;
};

internal Renderer renderer;

bool init_renderer_vulkan(VkInstance instance, VkSurfaceKHR surface, Memory_Arena* memory_arena)
{
    vulkan_context.device = NULL;
    vulkan_context.swapchain = NULL;
    vulkan_context.swapchain_image_count = 0;

    vulkan_context.memory_arena = memory_arena;
    vulkan_context.instance = instance;
    vulkan_context.surface = surface;

    #define VK_FUNCTION(function) function = (PFN_##function)vkGetInstanceProcAddr(vulkan_context.instance, #function);
        VK_FUNCTIONS_INSTANCE

    #if defined(DEBUG)
        VK_FUNCTIONS_DEBUG

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debug_callback;

        if (vkCreateDebugUtilsMessengerEXT(vulkan_context.instance, &debug_create_info, NULL, &debug_messenger) != VK_SUCCESS)
        {
            // TODO: Logging
            printf("Unable to set up Vulkan debug messenger!\n");
            return false;
        }
    #endif
    #undef VK_FUNCTION

    u32 device_count = 0;
    if (vkEnumeratePhysicalDevices(vulkan_context.instance, &device_count, NULL) != VK_SUCCESS || device_count == 0)
    {
        // TODO: Logging
        printf("Error enumerating Vulkan physical devices!\n");
        return false;
    }

    Memory_Arena_Marker memory_arena_marker = memory_arena_get_marker(vulkan_context.memory_arena);

    VkPhysicalDevice* physical_devices = memory_arena_reserve_array(vulkan_context.memory_arena, VkPhysicalDevice, device_count);
    if (vkEnumeratePhysicalDevices(vulkan_context.instance, &device_count, physical_devices) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Error enumerating Vulkan physical devices!\n");
        return false;
    }

    // NOTE: Selecting first device for now
    vulkan_context.physical_device = physical_devices[0];

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_context.physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties* queue_properties = memory_arena_reserve_array(vulkan_context.memory_arena, VkQueueFamilyProperties, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_context.physical_device, &queue_family_count, queue_properties);

    bool separate_transfer_queue = true;
    vulkan_context.graphics_queue_index = U32_MAX;
    vulkan_context.present_queue_index = U32_MAX;
    vulkan_context.transfer_queue_index = U32_MAX;

    for (u32 i = 0; i < queue_family_count; ++i)
    {
        if (vulkan_context.graphics_queue_index == U32_MAX && queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queue_properties[i].queueCount > 0)
        {
            vulkan_context.graphics_queue_index = i;
        }

        VkBool32 has_present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(vulkan_context.physical_device, i, vulkan_context.surface, &has_present_support);
        if (vulkan_context.present_queue_index == U32_MAX && has_present_support)
        {
            vulkan_context.present_queue_index = i;
        }

        if (vulkan_context.transfer_queue_index == U32_MAX &&
            queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
            (((vulkan_context.graphics_queue_index == i || vulkan_context.present_queue_index == i) && queue_properties[i].queueCount > 1) || queue_properties[i].queueCount > 0))
        {
            vulkan_context.transfer_queue_index = i;
        }

        if (vulkan_context.graphics_queue_index != U32_MAX && vulkan_context.present_queue_index != U32_MAX && vulkan_context.transfer_queue_index != U32_MAX)
        {
            break;
        }
    }

    memory_arena_free_to_marker(vulkan_context.memory_arena, memory_arena_marker);

    if (vulkan_context.transfer_queue_index == U32_MAX)
    {
        separate_transfer_queue = false;
        vulkan_context.transfer_queue_index = vulkan_context.graphics_queue_index;
    }

    if (vulkan_context.graphics_queue_index == U32_MAX || vulkan_context.present_queue_index == U32_MAX)
    {
        // TODO: Logging
        printf("Unable to find required Vulkan queue families!\n");
        return false;
    }

    u32 queue_info_count = 1;
    f32 queue_priorities[] = {1.0f, 1.0f};
    VkDeviceQueueCreateInfo queue_create_infos[3];

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = vulkan_context.graphics_queue_index;
    queue_create_info.queueCount = vulkan_context.graphics_queue_index == vulkan_context.transfer_queue_index ? 2 : 1;
    queue_create_info.pQueuePriorities = &queue_priorities[0];
    queue_create_infos[0] = queue_create_info;

    if (vulkan_context.present_queue_index != vulkan_context.graphics_queue_index)
    {
        queue_create_info.queueFamilyIndex = vulkan_context.present_queue_index;
        queue_create_info.queueCount = vulkan_context.present_queue_index == vulkan_context.transfer_queue_index ? 2 : 1;
        queue_create_infos[queue_info_count++] = queue_create_info;
    }

    if (vulkan_context.transfer_queue_index != vulkan_context.graphics_queue_index && vulkan_context.transfer_queue_index != vulkan_context.present_queue_index)
    {
        queue_create_info.queueFamilyIndex = vulkan_context.transfer_queue_index;
        queue_create_info.queueCount = 1;
        queue_create_infos[queue_info_count++] = queue_create_info;
    }

    vkGetPhysicalDeviceProperties(vulkan_context.physical_device, &vulkan_context.physical_device_properties);
    vkGetPhysicalDeviceFeatures(vulkan_context.physical_device, &vulkan_context.physical_device_features);
    vkGetPhysicalDeviceMemoryProperties(vulkan_context.physical_device, &vulkan_context.physical_device_memory_properties);

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = queue_info_count;
    device_create_info.pQueueCreateInfos = &queue_create_infos[0];
    device_create_info.pEnabledFeatures = &vulkan_context.physical_device_features;

    char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME};
    device_create_info.enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]);
    device_create_info.ppEnabledExtensionNames = &device_extensions[0];

    if (vkCreateDevice(vulkan_context.physical_device, &device_create_info, NULL, &vulkan_context.device) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan device!\n");
        return false;
    }

    #define VK_FUNCTION(function) function = (PFN_##function)vkGetDeviceProcAddr(vulkan_context.device, #function);
        VK_FUNCTIONS_DEVICE
    #undef VK_FUNCTION

    vkGetDeviceQueue(vulkan_context.device, vulkan_context.graphics_queue_index, 0, &vulkan_context.graphics_queue);
    vkGetDeviceQueue(vulkan_context.device, vulkan_context.present_queue_index, 0, &vulkan_context.present_queue);
    vkGetDeviceQueue(vulkan_context.device, vulkan_context.transfer_queue_index, (vulkan_context.transfer_queue_index == vulkan_context.graphics_queue_index || vulkan_context.transfer_queue_index == vulkan_context.present_queue_index) && separate_transfer_queue ? 1 : 0, &vulkan_context.transfer_queue);

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = STAGING_BUFFER_SIZE;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer.staging_buffer, &renderer.staging_memory_block))
    {
        // TODO: Logging
        printf("Failed to create Vulkan renderer staging buffer!\n");
        return false;
    }

    renderer.staging_arena = {renderer.staging_memory_block.base, 0, 0};

    buffer_create_info.size = MESH_BUFFER_SIZE;
    buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (!allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer.mesh_buffer, &renderer.mesh_memory_block))
    {
        // TODO: Logging
        printf("Failed to create Vulkan renderer mesh buffer!\n");
        return false;
    }
    return true;
}

bool recreate_swapchain(u32 width, u32 height)
{
    assert(vulkan_context.device != NULL && vulkan_context.surface != NULL);
    vkDeviceWaitIdle(vulkan_context.device);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_context.physical_device, vulkan_context.surface, &surface_capabilities) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to get Vulkan presentation surface capabilities!\n");
        return false;
    }

    u32 format_count = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_context.physical_device, vulkan_context.surface, &format_count, NULL) != VK_SUCCESS || format_count == 0)
    {
        // TODO: Logging
        printf("Error enumerating Vulkan presentation surface formats!\n");
        return false;
    }

    Memory_Arena_Marker memory_arena_marker = memory_arena_get_marker(vulkan_context.memory_arena);

    VkSurfaceFormatKHR* surface_formats = memory_arena_reserve_array(vulkan_context.memory_arena, VkSurfaceFormatKHR, format_count);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_context.physical_device, vulkan_context.surface, &format_count, surface_formats) != VK_SUCCESS || format_count == 0)
    {
        // TODO: Logging
        printf("Error enumerating Vulkan presentation surface formats!\n");
        return false;
    }

    u32 present_mode_count = 0;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_context.physical_device, vulkan_context.surface, &present_mode_count, NULL) != VK_SUCCESS || present_mode_count == 0)
    {
        // TODO: Logging
        printf("Error enumerating Vulkan present modes!\n");
        return false;
    }

    VkPresentModeKHR* present_modes = memory_arena_reserve_array(vulkan_context.memory_arena, VkPresentModeKHR, present_mode_count);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_context.physical_device, vulkan_context.surface, &present_mode_count, present_modes) != VK_SUCCESS || present_mode_count == 0)
    {
        // TODO: Logging
        printf("Error enumerating Vulkan present modes!\n");
        return false;
    }

    u32 image_count = min(surface_capabilities.minImageCount + 1, MAX_SWAPCHAIN_IMAGES);
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }

    vulkan_context.surface_format = surface_formats[0];
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        vulkan_context.surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    else
    {
        for (u32 i = 0; i < format_count; ++i)
        {
            if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                vulkan_context.surface_format = surface_formats[i];
                break;
            }
        }
    }

    vulkan_context.swapchain_extent = surface_capabilities.currentExtent;
    if (surface_capabilities.currentExtent.width == U32_MAX)
    {
        vulkan_context.swapchain_extent.width = max(surface_capabilities.minImageExtent.width, min(surface_capabilities.maxImageExtent.width, width));
        vulkan_context.swapchain_extent.height = max(surface_capabilities.minImageExtent.height, min(surface_capabilities.maxImageExtent.height, height));
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < present_mode_count; ++i)
    {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = present_modes[i];
            break;
        }
    }

    memory_arena_free_to_marker(vulkan_context.memory_arena, memory_arena_marker);

    VkSwapchainKHR old_swapchain = vulkan_context.swapchain;

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vulkan_context.surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = vulkan_context.surface_format.format;
    swapchain_create_info.imageColorSpace = vulkan_context.surface_format.colorSpace;
    swapchain_create_info.imageExtent = vulkan_context.swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = present_mode;
    swapchain_create_info.clipped = true;
    swapchain_create_info.oldSwapchain = old_swapchain;

    if (vkCreateSwapchainKHR(vulkan_context.device, &swapchain_create_info, NULL, &vulkan_context.swapchain) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan swapchain!\n");
        return false;
    }

    if (old_swapchain != NULL)
    {
        vkDestroySwapchainKHR(vulkan_context.device, old_swapchain, NULL);
    }

    for (u32 i = 0; i < vulkan_context.swapchain_image_count; ++i)
    {
        vkDestroyImageView(vulkan_context.device, vulkan_context.swapchain_image_views[i], NULL);
    }

    vkGetSwapchainImagesKHR(vulkan_context.device, vulkan_context.swapchain, &image_count, NULL);

    // This might fail if image_count is greater than MAX_SWAPCHAIN_IMAGES! I don't know!
    // This shouldn't be a problem since we will probably get exactly MAX_SWAPCHAIN_IMAGES images anyways
    vulkan_context.swapchain_image_count = min(image_count, MAX_SWAPCHAIN_IMAGES);
    vkGetSwapchainImagesKHR(vulkan_context.device, vulkan_context.swapchain, &image_count, &vulkan_context.swapchain_images[0]);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = vulkan_context.surface_format.format;
    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    for (u32 i = 0; i < vulkan_context.swapchain_image_count; ++i)
    {
        image_view_create_info.image = vulkan_context.swapchain_images[i];
        if (vkCreateImageView(vulkan_context.device, &image_view_create_info, NULL, &vulkan_context.swapchain_image_views[i]) != VK_SUCCESS)
        {
            // TODO: Logging
            printf("Unable to create Vulkan swapchain image view!\n");
            return false;
        }
    }

    return true;
}

void renderer_submit_frame(Frame_Parameters* frame_params)
{
    Frame_Resources* frame_resources = &renderer.frame_resources[frame_params->frame_number % MAX_FRAME_RESOURCES];

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame_resources->image_available_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frame_resources->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &frame_resources->render_finished_semaphore;

    if (vkQueueSubmit(vulkan_context.graphics_queue, 1, &submit_info, NULL) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Failed to submit Vulkan command buffer!\n");
        return;
    }

    VkPresentInfoKHR present_info = {};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame_resources->render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vulkan_context.swapchain;
    present_info.pImageIndices = &frame_resources->swapchain_image_index;

    VkResult result = vkQueuePresentKHR(vulkan_context.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        // TODO: Recreate swapchain/resources
    }
    else if (result != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to present Vulkan swapchain image!\n");
    }
}

void renderer_resize(u32 window_width, u32 window_height)
{
    // Note: Vulkan needs to be initialized
    if (vulkan_context.device == NULL || vulkan_context.surface == NULL)
    {
        return;
    }

    if (!recreate_swapchain(window_width, window_height))
    {
        // TODO: Logging
        printf("Error creating Vulkan swapchain!\n");
        return;
    }
}

