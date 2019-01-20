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

    VkRenderPass  swapchain_render_pass;
    VkFramebuffer swapchain_framebuffers[MAX_SWAPCHAIN_IMAGES];

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

    VkPipelineLayout pipeline_layout;
    VkPipeline       pipeline;

    Vulkan_Memory_Block staging_memory_block;
    Memory_Arena        staging_arena;
    VkBuffer            staging_buffer;

    Vulkan_Memory_Block mesh_memory_block;
    VkBuffer            mesh_buffer;
};

internal Renderer renderer;

bool recreate_swapchain(u32 width, u32 height);

bool init_renderer_vulkan(VkInstance instance, VkSurfaceKHR surface, u32 window_width, u32 window_height, Memory_Arena* memory_arena)
{
    vulkan_context = {};
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

    if (!recreate_swapchain(window_width, window_height))
    {
        // TODO: Logging
        printf("Error initializing Vulkan swapchain!\n");
        return false;
    }

    // TODO: Make it easier to create piplines!
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(vulkan_context.device, &pipeline_layout_create_info, NULL, &renderer.pipeline_layout) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan renderer pipeline layout!\n");
        return false;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = 2;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment;

    memory_arena_marker = memory_arena_get_marker(vulkan_context.memory_arena);

    // TODO: Use final read file implementation
    u8* vertex_shader_code;
    u64 vertex_shader_code_size;
    if (!DEBUG_read_file("shaders/vertex.spv", vulkan_context.memory_arena, &vertex_shader_code_size, &vertex_shader_code))
    {
        return false;
    }

    VkShaderModule vertex_shader_module, fragment_shader_module;
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = vertex_shader_code_size;
    shader_module_create_info.pCode = (u32*)vertex_shader_code;
    if (vkCreateShaderModule(vulkan_context.device, &shader_module_create_info, NULL, &vertex_shader_module) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan renderer vertex shader module\n");
        return false;
    }

    u8* fragment_shader_code;
    u64 fragment_shader_code_size;
    if (!DEBUG_read_file("shaders/fragment.spv", vulkan_context.memory_arena, &fragment_shader_code_size, &fragment_shader_code))
    {
        return false;
    }

    shader_module_create_info.codeSize = fragment_shader_code_size;
    shader_module_create_info.pCode = (u32*)fragment_shader_code;
    if (vkCreateShaderModule(vulkan_context.device, &shader_module_create_info, NULL, &fragment_shader_module) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan renderer fragment shader module\n");
        return false;
    }

    memory_arena_free_to_marker(vulkan_context.memory_arena, memory_arena_marker);

    VkPipelineShaderStageCreateInfo vertex_stage_create_info = {};
    vertex_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_stage_create_info.module = vertex_shader_module;
    vertex_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_stage_create_info = vertex_stage_create_info;
    fragment_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_stage_create_info.module = fragment_shader_module;

    VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_stage_create_info, fragment_stage_create_info};

    // NOTE: We are using the swapchain render pass created earlier.
    // There is a chance the render pass needs to be recreated (which we aren't doing).
    // In that case, the pipeline would probably need to be recreated as well.
    // However, the pipeline just needs to be compatible with other render passes,
    // so we are probably fine (the render pass won't change anyways).
    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.layout = renderer.pipeline_layout;
    pipeline_create_info.renderPass = vulkan_context.swapchain_render_pass;

    // NOTE: Ignoring pipeline cache for now
    if (vkCreateGraphicsPipelines(vulkan_context.device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &renderer.pipeline) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan renderer pipeline!\n");
        return false;
    }

    vkDestroyShaderModule(vulkan_context.device, vertex_shader_module, NULL);
    vkDestroyShaderModule(vulkan_context.device, fragment_shader_module, NULL);

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

    // NOTE: We are currently recreating the render pass when the swapchain is recreated.
    // This may not be necessary since the only parameter the render pass needs is the
    // swapchain's surface format (which will likely always be the same).
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = vulkan_context.surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    if (vkCreateRenderPass(vulkan_context.device, &render_pass_create_info, NULL, &vulkan_context.swapchain_render_pass) != VK_SUCCESS)
    {
        // TODO: Logging
        printf("Unable to create Vulkan swapchain render pass!\n");
        return false;
    }

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = vulkan_context.surface_format.format;
    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.renderPass = vulkan_context.swapchain_render_pass;
    framebuffer_create_info.width = vulkan_context.swapchain_extent.width;
    framebuffer_create_info.height = vulkan_context.swapchain_extent.height;
    framebuffer_create_info.layers = 1;

    for (u32 i = 0; i < vulkan_context.swapchain_image_count; ++i)
    {
        image_view_create_info.image = vulkan_context.swapchain_images[i];
        if (vkCreateImageView(vulkan_context.device, &image_view_create_info, NULL, &vulkan_context.swapchain_image_views[i]) != VK_SUCCESS)
        {
            // TODO: Logging
            printf("Unable to create Vulkan swapchain image view!\n");
            return false;
        }

        if (vulkan_context.swapchain_framebuffers[i] != NULL)
        {
            vkDestroyFramebuffer(vulkan_context.device, vulkan_context.swapchain_framebuffers[i], NULL);
        }

        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = &vulkan_context.swapchain_image_views[i];
        if (vkCreateFramebuffer(vulkan_context.device, &framebuffer_create_info, NULL, &vulkan_context.swapchain_framebuffers[i]) != VK_SUCCESS)
        {
            // TODO: Logging
            printf("Unable to create Vulkan swapchain framebuffer!\n");
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
        printf("Error recreating Vulkan swapchain!\n");
        return;
    }
}

