#include "ツ.vulkan.h"

#define VK_FUNCTION(function) PFN_##function function;
    VK_FUNCTIONS_DEBUG
    VK_FUNCTIONS_INSTANCE
#undef VK_FUNCTION

internal VkDebugUtilsMessengerEXT debug_messenger;

internal VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    // TODO: Logging
    // TODO: Better log message
    printf("Vulkan: %s\n", callback_data->pMessage);
    return false;
}

struct Vulkan_Context
{
    VkInstance                 instance;
    VkDevice                   device;
    VkPhysicalDevice           physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    VkPhysicalDeviceFeatures   physical_device_features;

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
    VkImage*           swapchain_images;
    VkImageView*       swapchain_image_views;
    u32                swapchain_image_count;
};

internal Vulkan_Context vulkan_context;

bool init_renderer_vulkan(VkInstance instance, VkSurfaceKHR surface, Memory_Arena* memory_arena)
{
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

    VkPhysicalDevice* physical_devices = memory_arena_reserve_array(memory_arena, VkPhysicalDevice, device_count);
    if (vkEnumeratePhysicalDevices(vulkan_context.instance, &device_count, physical_devices) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Error enumerating Vulkan physical devices!\n");
        return false;
    }

    return true;
}

