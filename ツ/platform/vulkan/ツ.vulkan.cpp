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

    // TODO: Memory arena temporary memory
    VkPhysicalDevice* physical_devices = memory_arena_reserve_array(memory_arena, VkPhysicalDevice, device_count);
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

    // TODO: Memory arena temporary memory
    VkQueueFamilyProperties* queue_properties = memory_arena_reserve_array(memory_arena, VkQueueFamilyProperties, queue_family_count);
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

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = queue_info_count;
    device_create_info.pQueueCreateInfos = &queue_create_infos[0];
    device_create_info.pEnabledFeatures = &vulkan_context.physical_device_features;

    char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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

    return true;
}

