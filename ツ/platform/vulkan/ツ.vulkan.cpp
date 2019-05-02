#define VK_FUNCTION(function) PFN_##function function;
    VK_FUNCTIONS_DEBUG
    VK_FUNCTIONS_INSTANCE
    VK_FUNCTIONS_DEVICE
#undef VK_FUNCTION

#if defined(DEBUG)
    internal VkDebugUtilsMessengerEXT debug_messenger;

    internal b32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
    {
        // TODO: Logging
        // TODO: Better log message
        DEBUG_printf("Vulkan: %s\n", callback_data->pMessage);
        return false;
    }
#endif

#if defined(DEBUG)
    #define VK_CALL(result) assert((result) == VK_SUCCESS)
#else
   #define VK_CALL(result) result
#endif

#define MAX_SWAPCHAIN_IMAGES 3
#define MAX_FRAME_RESOURCES  3

struct Vulkan_Context
{
    VkInstance                       instance;
    VkDevice                         device;
    VkPhysicalDevice                 physical_device;
    VkPhysicalDeviceProperties       physical_device_properties;
    VkPhysicalDeviceFeatures         physical_device_features;
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;

    VkQueue graphics_compute_queue;
    u32     graphics_compute_queue_index;

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

internal Vulkan_Memory_Block allocate_vulkan_memory_block(u64 size, u32 memory_type_index)
{
    Vulkan_Memory_Block memory_block;

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = size;
    allocate_info.memoryTypeIndex = memory_type_index;
    VK_CALL(vkAllocateMemory(vulkan_context.device, &allocate_info, NULL, &memory_block.device_memory));

    memory_block.memory_type = vulkan_context.physical_device_memory_properties.memoryTypes[memory_type_index];
    memory_block.memory_type_index = memory_type_index;
    memory_block.size = size;
    memory_block.used = 0;

    if (memory_block.memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(vulkan_context.device, memory_block.device_memory, 0, VK_WHOLE_SIZE, 0, (void**)&memory_block.base);
    }
    return memory_block;
}

internal VkBuffer vulkan_memory_block_reserve_buffer(Vulkan_Memory_Block* memory_block, VkBufferCreateInfo* buffer_create_info)
{
    VkBuffer buffer;
    VK_CALL(vkCreateBuffer(vulkan_context.device, buffer_create_info, NULL, &buffer));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vulkan_context.device, buffer, &memory_requirements);

    // NOTE: We will generally only have one VkBuffer per allocation (memory_block).
    // So there is no reason for doing this since it will use the entire buffer and the starting offset is 0.
    u64 aligned_offset = memory_block->used + memory_requirements.alignment / 2;
    aligned_offset -= aligned_offset % memory_requirements.alignment;

    assert((aligned_offset + memory_requirements.size <= memory_block->size) && (memory_requirements.memoryTypeBits & (1 << memory_block->memory_type_index)));
    VK_CALL(vkBindBufferMemory(vulkan_context.device, buffer, memory_block->device_memory, aligned_offset));

    memory_block->used = aligned_offset + memory_requirements.size;
    return buffer;
}

internal void allocate_vulkan_buffer(VkBufferCreateInfo* buffer_create_info, VkMemoryPropertyFlags memory_properties, VkBuffer* buffer, Vulkan_Memory_Block* memory_block)
{
    VK_CALL(vkCreateBuffer(vulkan_context.device, buffer_create_info, NULL, buffer));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vulkan_context.device, *buffer, &memory_requirements);

    *memory_block = allocate_vulkan_memory_block(memory_requirements.size, get_memory_type_index(memory_requirements.memoryTypeBits, memory_properties));
    VK_CALL(vkBindBufferMemory(vulkan_context.device, *buffer, memory_block->device_memory, 0));

    memory_block->used += memory_requirements.size;
}

internal VkImage vulkan_memory_block_reserve_image(Vulkan_Memory_Block* memory_block, VkImageCreateInfo* image_create_info)
{
    VkImage image;
    VK_CALL(vkCreateImage(vulkan_context.device, image_create_info, NULL, &image));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_context.device, image, &memory_requirements);

    // TODO: Need to manage this when we have to free memory
    u64 aligned_offset = memory_block->used + memory_requirements.alignment / 2;
    aligned_offset -= aligned_offset % memory_requirements.alignment;

    assert((aligned_offset + memory_requirements.size <= memory_block->size) && (memory_requirements.memoryTypeBits & (1 << memory_block->memory_type_index)));
    VK_CALL(vkBindImageMemory(vulkan_context.device, image, memory_block->device_memory, aligned_offset));

    memory_block->used = aligned_offset + memory_requirements.size;
    return image;
}

internal void vulkan_memory_block_reserve_image(Vulkan_Memory_Block* memory_block, VkImage image)
{
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_context.device, image, &memory_requirements);

    // TODO: Need to manage this when we have to free memory
    u64 aligned_offset = memory_block->used + memory_requirements.alignment / 2;
    aligned_offset -= aligned_offset % memory_requirements.alignment;

    assert((aligned_offset + memory_requirements.size <= memory_block->size) && (memory_requirements.memoryTypeBits & (1 << memory_block->memory_type_index)));
    VK_CALL(vkBindImageMemory(vulkan_context.device, image, memory_block->device_memory, aligned_offset));

    memory_block->used = aligned_offset + memory_requirements.size;
}

internal void allocate_vulkan_image(VkImageCreateInfo* image_create_info, VkMemoryPropertyFlags memory_properties, VkImage* image, Vulkan_Memory_Block* memory_block)
{
    VK_CALL(vkCreateImage(vulkan_context.device, image_create_info, NULL, image));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_context.device, *image, &memory_requirements);

    *memory_block = allocate_vulkan_memory_block(memory_requirements.size, get_memory_type_index(memory_requirements.memoryTypeBits, memory_properties));
    VK_CALL(vkBindImageMemory(vulkan_context.device, *image, memory_block->device_memory, 0));

    memory_block->used += memory_requirements.size;
}

struct Vulkan_Buffer
{
    u32 offset;
};

struct Vulkan_Texture
{
    VkImage     image;
    VkImageView image_view;
};

#define MAX_2D_TEXTURES          2048
#define MAX_DRAW_CALLS_PER_FRAME 16384
#define MAX_INSTANCES_PER_FRAME  (MAX_DRAW_CALLS_PER_FRAME * 2)
#define MAX_LIGHTS               1024
#define MAX_LIGHTS_PER_CLUSTER   64

struct Draw_Instance_Data
{
    u32 xform_index;
    u32 material_index;
};

struct Frame_Uniforms
{
    m4x4 view;
    m4x4 projection;
    m4x4 view_projection;
    m4x4 inverse_projection;
    m4x4 inverse_view_projection;

    m4x4 DEBUG_view;

    v4  camera_position;
    v4  voxel_grid_origin_resolution;
    uv4 cluster_grid_resolution_size;

    v2 inverse_render_dimensions;

    f32 voxel_size;
    f32 voxel_ray_step_size;

    f32 camera_near;
    f32 camera_far;

    u32 light_count;
};

// TODO: Fix transfer queue bug!!!
#define TRANSFER_MEMORY_SIZE megabytes(128)
#define MESH_MEMORY_SIZE     megabytes(256)
#define TEXTURE_MEMORY_SIZE  megabytes(256)

#define MAX_MATERIALS            MAX_INSTANCES_PER_FRAME
#define MAX_XFORMS               MAX_INSTANCES_PER_FRAME
#define MATERIALS_MEMORY_SIZE    (MAX_MATERIALS * sizeof(Material))
#define XFORMS_MEMORY_SIZE       (MAX_XFORMS * sizeof(m4x4))
#define MAX_CLUSTERS_MEMORY_SIZE megabytes(1)

#define DRAW_CALLS_MEMORY_SIZE     (MAX_DRAW_CALLS_PER_FRAME * sizeof(VkDrawIndexedIndirectCommand) + sizeof(u32))
#define DRAW_INSTANCE_MEMORY_SIZE  (MAX_INSTANCES_PER_FRAME * sizeof(Draw_Instance_Data))
#define FRAME_UNIFORMS_MEMORY_SIZE (sizeof(Frame_Uniforms))
#define LIGHT_STORAGE_MEMORY_SIZE  (MAX_LIGHTS * sizeof(Light))

struct Frame_Resources
{
    VkCommandBuffer graphics_compute_command_buffer;
    VkFence         graphics_compute_command_buffer_fence;
    b32             should_record_command_buffer;

    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    u32         swapchain_image_index;

    VkDescriptorSet descriptor_set;

    Memory_Arena draw_command_arena;
    u32*         draw_call_count;
    u64          draw_command_offset;
    u64          draw_call_count_offset;

    Memory_Arena draw_instance_arena;
    u32          draw_instance_count;
    u64          draw_instance_offset;

    Frame_Uniforms* uniforms;
    u64             uniforms_offset;

    Memory_Arena   light_arena;
    u64            light_storage_offset;

    // NOTE: This is the total space the draw commands, instances, and uniforms take.
    // (Including extra padding to align to minUniformBufferOffsetAlignment)
    u64 data_memory_size;
};

struct Renderer
{
    Frame_Resources frame_resources[MAX_FRAME_RESOURCES];

    VkPipelineLayout global_pipeline_layout;
    VkPipeline       clusterizer_pipeline;
    VkPipeline       cluster_light_culling_pipeline;
    VkPipeline       main_pipeline;
    VkPipeline       voxelizer_pipeline;
    VkPipeline       voxelizer_compute_pipeline;
    VkPipeline       shading_compute_pipeline;
    VkPipeline       final_pass_pipeline;

    VkCommandPool    graphics_command_pool;

    Vulkan_Memory_Block transfer_memory_block;
    VkBuffer            transfer_buffer;

    Renderer_Transfer_Queue transfer_queue;
    VkCommandPool           transfer_command_pool;
    VkCommandBuffer         transfer_command_buffer;
    VkFence                 transfer_command_buffer_fence;

    Vulkan_Memory_Block frame_resources_memory_block;
    VkBuffer            frame_resources_buffer;

    Vulkan_Memory_Block mesh_memory_block;
    VkBuffer            mesh_buffer;
    u64                 mesh_buffer_used;

    Vulkan_Memory_Block storage_memory_block;
    VkBuffer            storage_buffer;
    u64                 storage_materials_offset;
    u64                 storage_xforms_offset;

    Vulkan_Memory_Block texture_memory_block;
    u64                 texture_memory_used;

    Vulkan_Memory_Block framebuffer_memory_block;
    VkFramebuffer       main_framebuffer;
    Vulkan_Texture      main_color_attachment;
    Vulkan_Texture      main_depth_attachment;

    Vulkan_Memory_Block cluster_storage_memory_block;
    VkBuffer            cluster_storage_buffer;
    b32                 cluster_grid_needs_update;

    Vulkan_Memory_Block voxel_storage_memory_block;
    VkBuffer            voxel_storage_buffer;
    Vulkan_Memory_Block voxel_image_memory_block;
    Vulkan_Texture      voxel_texture;
    VkSampler           voxel_texture_sampler;

    VkSampler texture_2d_sampler;
    u32       max_2d_textures;

    Vulkan_Buffer*  mesh_buffers;
    Vulkan_Texture* textures_2d;

    VkDescriptorPool      descriptor_pool;
    VkDescriptorSetLayout global_descriptor_set_layout;
    VkDescriptorSetLayout per_frame_descriptor_set_layout;
    VkDescriptorSet       global_descriptor_set;

    VkRenderPass main_render_pass;
    VkRenderPass voxelization_render_pass;

    VkFramebuffer voxelization_pass_framebuffer;

    uv2 render_dimensions;
    uv3 cluster_grid_resolution;
    uv3 voxel_grid_resolution;

    VkPipeline DEBUG_cluster_visualizer_pipeline;
    b32        draw_debug_cluster_grid;

    VkPipeline DEBUG_voxel_visualizer_pipeline;
    b32        draw_debug_voxel_grid;

    Frame_Resources* current_render_frame;
};

internal Renderer renderer;

internal void recreate_swapchain(u32 window_width, u32 window_height)
{
    assert(vulkan_context.device != NULL && vulkan_context.surface != NULL);
    vkDeviceWaitIdle(vulkan_context.device);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_context.physical_device, vulkan_context.surface, &surface_capabilities));

    u32 format_count = 0;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_context.physical_device, vulkan_context.surface, &format_count, NULL));
    assert(format_count != 0);

    Memory_Arena_Marker memory_arena_marker = memory_arena_get_marker(vulkan_context.memory_arena);

    VkSurfaceFormatKHR* surface_formats = memory_arena_reserve_array(vulkan_context.memory_arena, VkSurfaceFormatKHR, format_count);
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_context.physical_device, vulkan_context.surface, &format_count, surface_formats));

    u32 present_mode_count = 0;
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_context.physical_device, vulkan_context.surface, &present_mode_count, NULL));
    assert(present_mode_count != 0);

    VkPresentModeKHR* present_modes = memory_arena_reserve_array(vulkan_context.memory_arena, VkPresentModeKHR, present_mode_count);
    VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_context.physical_device, vulkan_context.surface, &present_mode_count, present_modes));

    u32 image_count = min(surface_capabilities.minImageCount + 1, MAX_SWAPCHAIN_IMAGES);
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }

    vulkan_context.surface_format = surface_formats[0];
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        vulkan_context.surface_format = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    else
    {
        for (u32 i = 0; i < format_count; ++i)
        {
            if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                vulkan_context.surface_format = surface_formats[i];
                break;
            }
        }
    }

    vulkan_context.swapchain_extent = surface_capabilities.currentExtent;
    if (surface_capabilities.currentExtent.width == U32_MAX)
    {
        vulkan_context.swapchain_extent.width = max(surface_capabilities.minImageExtent.width, min(surface_capabilities.maxImageExtent.width, window_width));
        vulkan_context.swapchain_extent.height = max(surface_capabilities.minImageExtent.height, min(surface_capabilities.maxImageExtent.height, window_height));
    }

    // NOTE: Right now, we are assuming that we will get mailbox present mode.
    // If we don't, then we should not triple buffer. (ie. MAX_FRAME_RESOURCES should be 2 instead of 3)
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

    VK_CALL(vkCreateSwapchainKHR(vulkan_context.device, &swapchain_create_info, NULL, &vulkan_context.swapchain));
    if (old_swapchain != NULL)
    {
        vkDestroySwapchainKHR(vulkan_context.device, old_swapchain, NULL);
    }

    for (u32 i = 0; i < vulkan_context.swapchain_image_count; ++i)
    {
        vkDestroyImageView(vulkan_context.device, vulkan_context.swapchain_image_views[i], NULL);
    }

    vkGetSwapchainImagesKHR(vulkan_context.device, vulkan_context.swapchain, &image_count, NULL);

    // NOTE: This might fail if image_count is greater than MAX_SWAPCHAIN_IMAGES! I don't know!
    // This shouldn't be a problem since we will probably get exactly MAX_SWAPCHAIN_IMAGES images anyways
    vulkan_context.swapchain_image_count = min(image_count, MAX_SWAPCHAIN_IMAGES);
    vkGetSwapchainImagesKHR(vulkan_context.device, vulkan_context.swapchain, &image_count, &vulkan_context.swapchain_images[0]);

    VkAttachmentDescription color_attachment = {};
    color_attachment.format = vulkan_context.surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;
    VK_CALL(vkCreateRenderPass(vulkan_context.device, &render_pass_create_info, NULL, &vulkan_context.swapchain_render_pass));

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = vulkan_context.surface_format.format;
    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = vulkan_context.swapchain_render_pass;
    framebuffer_create_info.width = vulkan_context.swapchain_extent.width;
    framebuffer_create_info.height = vulkan_context.swapchain_extent.height;
    framebuffer_create_info.layers = 1;

    for (u32 i = 0; i < vulkan_context.swapchain_image_count; ++i)
    {
        image_view_create_info.image = vulkan_context.swapchain_images[i];
        VK_CALL(vkCreateImageView(vulkan_context.device, &image_view_create_info, NULL, &vulkan_context.swapchain_image_views[i]));

        if (vulkan_context.swapchain_framebuffers[i] != NULL)
        {
            vkDestroyFramebuffer(vulkan_context.device, vulkan_context.swapchain_framebuffers[i], NULL);
        }

        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = &vulkan_context.swapchain_image_views[i];
        VK_CALL(vkCreateFramebuffer(vulkan_context.device, &framebuffer_create_info, NULL, &vulkan_context.swapchain_framebuffers[i]));
    }
}

internal void recreate_main_framebuffer(uv2 render_dimensions)
{
    renderer.render_dimensions = render_dimensions;

    VkFormat main_depth_format = VK_FORMAT_D32_SFLOAT;
    VkFormat main_color_format = VK_FORMAT_R16G16B16A16_SFLOAT;

    // TODO: Free all attachment images and image views if they were previously allocated. Either reuse memory block (if new attachments size < old size) or free and reallocate.
    // TODO: Use dedicated allocation?
    VkImageCreateInfo attachment_image_create_info = {};
    attachment_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    attachment_image_create_info.imageType = VK_IMAGE_TYPE_2D;
    attachment_image_create_info.extent = {renderer.render_dimensions.x, renderer.render_dimensions.y, 1};
    attachment_image_create_info.mipLevels = 1;
    attachment_image_create_info.arrayLayers = 1;
    attachment_image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    attachment_image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    {
        attachment_image_create_info.format = main_depth_format;
        attachment_image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VK_CALL(vkCreateImage(vulkan_context.device, &attachment_image_create_info, NULL, &renderer.main_depth_attachment.image));
    }
    {
        attachment_image_create_info.format = main_color_format;
        attachment_image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VK_CALL(vkCreateImage(vulkan_context.device, &attachment_image_create_info, NULL, &renderer.main_color_attachment.image));
    }

    VkMemoryRequirements main_color_requirements, main_depth_requirements;
    vkGetImageMemoryRequirements(vulkan_context.device, renderer.main_color_attachment.image, &main_color_requirements);
    vkGetImageMemoryRequirements(vulkan_context.device, renderer.main_depth_attachment.image, &main_depth_requirements);

    renderer.framebuffer_memory_block = allocate_vulkan_memory_block(main_color_requirements.size + main_depth_requirements.size, get_memory_type_index(main_color_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

    vulkan_memory_block_reserve_image(&renderer.framebuffer_memory_block, renderer.main_color_attachment.image);
    vulkan_memory_block_reserve_image(&renderer.framebuffer_memory_block, renderer.main_depth_attachment.image);

    VkImageViewCreateInfo attachment_image_view_create_info = {};
    attachment_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    attachment_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    attachment_image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    {
        attachment_image_view_create_info.format = main_color_format;
        attachment_image_view_create_info.image = renderer.main_color_attachment.image;
        VK_CALL(vkCreateImageView(vulkan_context.device, &attachment_image_view_create_info, NULL, &renderer.main_color_attachment.image_view));
    }
    {
        attachment_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        attachment_image_view_create_info.format = main_depth_format;
        attachment_image_view_create_info.image = renderer.main_depth_attachment.image;
        VK_CALL(vkCreateImageView(vulkan_context.device, &attachment_image_view_create_info, NULL, &renderer.main_depth_attachment.image_view));
    }

    VkDescriptorImageInfo main_color_image_info = {NULL, renderer.main_color_attachment.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet main_color_image_write = {};
    main_color_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    main_color_image_write.dstSet = renderer.global_descriptor_set;
    main_color_image_write.dstBinding = 4;
    main_color_image_write.descriptorCount = 1;
    main_color_image_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    main_color_image_write.pImageInfo = &main_color_image_info;

    VkDescriptorImageInfo depth_image_info = {NULL, renderer.main_depth_attachment.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet depth_image_write = main_color_image_write;
    depth_image_write.dstBinding = 5;
    depth_image_write.pImageInfo = &depth_image_info;

    VkWriteDescriptorSet descriptor_writes[] = {main_color_image_write, depth_image_write};
    vkUpdateDescriptorSets(vulkan_context.device, array_count(descriptor_writes), descriptor_writes, 0, NULL);

    VkAttachmentDescription color_attachment_description = {};
    color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    color_attachment_description.format = main_color_format;

    VkAttachmentDescription depth_attachment_description = color_attachment_description;
    depth_attachment_description.format = main_depth_format;

    VkAttachmentDescription attachment_descriptions[] = {color_attachment_description, depth_attachment_description};
    VkAttachmentReference color_attachment_reference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_attachment_reference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription main_subpass = {};
    main_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    main_subpass.colorAttachmentCount = 1;
    main_subpass.pColorAttachments = &color_attachment_reference;
    main_subpass.pDepthStencilAttachment = &depth_attachment_reference;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = array_count(attachment_descriptions);
    render_pass_create_info.pAttachments = attachment_descriptions;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &main_subpass;
    VK_CALL(vkCreateRenderPass(vulkan_context.device, &render_pass_create_info, NULL, &renderer.main_render_pass));

    VkImageView attachments[] = {renderer.main_color_attachment.image_view, renderer.main_depth_attachment.image_view};
    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = renderer.main_render_pass;
    framebuffer_create_info.width = renderer.render_dimensions.x;
    framebuffer_create_info.height = renderer.render_dimensions.y;
    framebuffer_create_info.layers = 1;
    framebuffer_create_info.attachmentCount = array_count(attachments);
    framebuffer_create_info.pAttachments = attachments;
    VK_CALL(vkCreateFramebuffer(vulkan_context.device, &framebuffer_create_info, NULL, &renderer.main_framebuffer));
}

internal void recreate_cluster_grid(uv3 cluster_grid_resolution)
{
    renderer.draw_debug_cluster_grid = false;
    renderer.cluster_grid_resolution = cluster_grid_resolution;

    u32 cluster_count = renderer.cluster_grid_resolution.x * renderer.cluster_grid_resolution.y * renderer.cluster_grid_resolution.z;
    u64 cluster_aabb_size = cluster_count * sizeof(v4) * 2;
    u64 cluster_light_index_list_size = cluster_count * MAX_LIGHTS_PER_CLUSTER * sizeof(u32) + sizeof(u32);
    u64 cluster_light_info_size = cluster_count * sizeof(u32) * 2;

    u64 cluster_light_index_list_offset = (cluster_aabb_size + vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment - 1) & -(s64)vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment;
    u64 cluster_light_info_offset = ((cluster_light_index_list_offset + cluster_light_index_list_size) + vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment - 1) & -(s64)vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment;
    u64 cluster_storage_buffer_size = cluster_light_info_offset + cluster_light_info_size;

    // TODO: Free current cluster grid related resources before recreating new ones!
    // TODO: Use dedicated allocation extension?
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = cluster_storage_buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer.cluster_storage_buffer, &renderer.cluster_storage_memory_block);

    VkDescriptorBufferInfo cluster_aabbs_buffer_info = {renderer.cluster_storage_buffer, 0, cluster_aabb_size};
    VkWriteDescriptorSet cluster_aabbs_storage_write = {};
    cluster_aabbs_storage_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    cluster_aabbs_storage_write.dstSet = renderer.global_descriptor_set;
    cluster_aabbs_storage_write.dstBinding = 9;
    cluster_aabbs_storage_write.descriptorCount = 1;
    cluster_aabbs_storage_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cluster_aabbs_storage_write.pBufferInfo = &cluster_aabbs_buffer_info;

    VkDescriptorBufferInfo cluster_light_index_list_buffer_info = {renderer.cluster_storage_buffer, cluster_light_index_list_offset, cluster_light_index_list_size};
    VkWriteDescriptorSet cluster_light_index_list_storage_write = cluster_aabbs_storage_write;
    cluster_light_index_list_storage_write.dstBinding = 10;
    cluster_light_index_list_storage_write.pBufferInfo = &cluster_light_index_list_buffer_info;

    VkDescriptorBufferInfo cluster_light_info_buffer_info = {renderer.cluster_storage_buffer, cluster_light_info_offset, cluster_light_info_size};
    VkWriteDescriptorSet cluster_light_info_storage_write = cluster_aabbs_storage_write;
    cluster_light_info_storage_write.dstBinding = 11;
    cluster_light_info_storage_write.pBufferInfo = &cluster_light_info_buffer_info;

    VkWriteDescriptorSet descriptor_writes[] = {cluster_aabbs_storage_write, cluster_light_index_list_storage_write, cluster_light_info_storage_write};
    vkUpdateDescriptorSets(vulkan_context.device, array_count(descriptor_writes), descriptor_writes, 0, NULL);

    renderer.cluster_grid_needs_update = true;
}

internal void recreate_voxel_grid(u32 voxel_grid_resolution)
{
    renderer.draw_debug_voxel_grid = true;
    renderer.voxel_grid_resolution = {voxel_grid_resolution, voxel_grid_resolution, voxel_grid_resolution};

    VkSubpassDescription voxelization_subpass = {};
    voxelization_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkSubpassDependency voxelization_subpass_dependency = {};
    voxelization_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    voxelization_subpass_dependency.dstSubpass = 0;
    voxelization_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    voxelization_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    voxelization_subpass_dependency.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    voxelization_subpass_dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &voxelization_subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &voxelization_subpass_dependency;
    VK_CALL(vkCreateRenderPass(vulkan_context.device, &render_pass_create_info, NULL, &renderer.voxelization_render_pass));

    // NOTE: This framebuffer is needed even though we don't actually use any attachments
    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = renderer.voxelization_render_pass;
    framebuffer_create_info.width = renderer.voxel_grid_resolution.x;
    framebuffer_create_info.height = renderer.voxel_grid_resolution.x;
    framebuffer_create_info.layers = 1;
    VK_CALL(vkCreateFramebuffer(vulkan_context.device, &framebuffer_create_info, NULL, &renderer.voxelization_pass_framebuffer));

    // TODO: Free current voxel grid related resources before recreating new ones!
    // TODO: Use dedicated allocation extension?
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = renderer.voxel_grid_resolution.x * renderer.voxel_grid_resolution.y * renderer.voxel_grid_resolution.z * sizeof(u32);
    buffer_create_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer.voxel_storage_buffer, &renderer.voxel_storage_memory_block);

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_3D;
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_create_info.extent = {renderer.voxel_grid_resolution.x, renderer.voxel_grid_resolution.y, renderer.voxel_grid_resolution.z};
    image_create_info.mipLevels = most_significant_bit_index(max(max(renderer.voxel_grid_resolution.x, renderer.voxel_grid_resolution.y), renderer.voxel_grid_resolution.z)) + 1;;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    allocate_vulkan_image(&image_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer.voxel_texture.image, &renderer.voxel_image_memory_block);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = renderer.voxel_texture.image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
    image_view_create_info.format = image_create_info.format;
    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, image_create_info.mipLevels, 0, image_create_info.arrayLayers};
    VK_CALL(vkCreateImageView(vulkan_context.device, &image_view_create_info, NULL, &renderer.voxel_texture.image_view));

    VkDescriptorBufferInfo voxel_grid_buffer_info = {renderer.voxel_storage_buffer, 0, renderer.voxel_storage_memory_block.size};
    VkWriteDescriptorSet voxel_grid_storage_write = {};
    voxel_grid_storage_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    voxel_grid_storage_write.dstSet = renderer.global_descriptor_set;
    voxel_grid_storage_write.dstBinding = 6;
    voxel_grid_storage_write.descriptorCount = 1;
    voxel_grid_storage_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    voxel_grid_storage_write.pBufferInfo = &voxel_grid_buffer_info;

    VkDescriptorImageInfo voxel_grid_image_info = {renderer.voxel_texture_sampler, renderer.voxel_texture.image_view, VK_IMAGE_LAYOUT_GENERAL};
    VkWriteDescriptorSet voxel_grid_image_write = voxel_grid_storage_write;
    voxel_grid_image_write.dstBinding = 7;
    voxel_grid_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    voxel_grid_image_write.pImageInfo = &voxel_grid_image_info;
    voxel_grid_image_write.pBufferInfo = NULL;

    VkWriteDescriptorSet voxel_texture_write = voxel_grid_image_write;
    voxel_texture_write.dstBinding = 8;
    voxel_texture_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    VkWriteDescriptorSet descriptor_writes[] = {voxel_grid_storage_write, voxel_grid_image_write, voxel_texture_write};
    vkUpdateDescriptorSets(vulkan_context.device, array_count(descriptor_writes), descriptor_writes, 0, NULL);
}

void init_vulkan_renderer(VkInstance instance, VkSurfaceKHR surface, u32 window_width, u32 window_height, Memory_Arena* memory_arena)
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

        VK_CALL(vkCreateDebugUtilsMessengerEXT(vulkan_context.instance, &debug_create_info, NULL, &debug_messenger));
    #endif
    #undef VK_FUNCTION

    u32 device_count = 0;
    VK_CALL(vkEnumeratePhysicalDevices(vulkan_context.instance, &device_count, NULL));
    assert(device_count != 0);

    Memory_Arena_Marker memory_arena_marker = memory_arena_get_marker(vulkan_context.memory_arena);

    VkPhysicalDevice* physical_devices = memory_arena_reserve_array(vulkan_context.memory_arena, VkPhysicalDevice, device_count);
    VK_CALL(vkEnumeratePhysicalDevices(vulkan_context.instance, &device_count, physical_devices));

    // NOTE: Selecting first device for now
    vulkan_context.physical_device = physical_devices[0];

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_context.physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties* queue_properties = memory_arena_reserve_array(vulkan_context.memory_arena, VkQueueFamilyProperties, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_context.physical_device, &queue_family_count, queue_properties);

    b32 separate_transfer_queue = true;
    vulkan_context.graphics_compute_queue_index = U32_MAX;
    vulkan_context.present_queue_index = U32_MAX;
    vulkan_context.transfer_queue_index = U32_MAX;

    for (u32 i = 0; i < queue_family_count; ++i)
    {
        if (vulkan_context.graphics_compute_queue_index == U32_MAX && (queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && queue_properties[i].queueCount > 0)
        {
            vulkan_context.graphics_compute_queue_index = i;
        }

        b32 has_present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(vulkan_context.physical_device, i, vulkan_context.surface, &has_present_support);
        if (vulkan_context.present_queue_index == U32_MAX && has_present_support)
        {
            vulkan_context.present_queue_index = i;
        }

        if (vulkan_context.transfer_queue_index == U32_MAX &&
            queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
            (((vulkan_context.graphics_compute_queue_index == i || vulkan_context.present_queue_index == i) && queue_properties[i].queueCount > 1) || queue_properties[i].queueCount > 0))
        {
            vulkan_context.transfer_queue_index = i;
        }

        if (vulkan_context.graphics_compute_queue_index != U32_MAX && vulkan_context.present_queue_index != U32_MAX && vulkan_context.transfer_queue_index != U32_MAX)
        {
            break;
        }
    }

    memory_arena_free_to_marker(vulkan_context.memory_arena, memory_arena_marker);

    if (vulkan_context.transfer_queue_index == U32_MAX)
    {
        separate_transfer_queue = false;
        vulkan_context.transfer_queue_index = vulkan_context.graphics_compute_queue_index;
    }
    assert(vulkan_context.graphics_compute_queue_index != U32_MAX && vulkan_context.present_queue_index != U32_MAX);

    u32 queue_info_count = 1;
    f32 queue_priorities[] = {1.0f, 1.0f};
    VkDeviceQueueCreateInfo queue_create_infos[3];

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = vulkan_context.graphics_compute_queue_index;
    queue_create_info.queueCount = vulkan_context.graphics_compute_queue_index == vulkan_context.transfer_queue_index ? 2 : 1;
    queue_create_info.pQueuePriorities = &queue_priorities[0];
    queue_create_infos[0] = queue_create_info;

    // NOTE: If the present queue is different from the graphics queue we need to handle submitting a frame a little differently.
    // Most likely the present queue and graphics queue will be the same.
    if (vulkan_context.present_queue_index != vulkan_context.graphics_compute_queue_index)
    {
        queue_create_info.queueFamilyIndex = vulkan_context.present_queue_index;
        queue_create_info.queueCount = vulkan_context.present_queue_index == vulkan_context.transfer_queue_index ? 2 : 1;
        queue_create_infos[queue_info_count++] = queue_create_info;
    }

    if (vulkan_context.transfer_queue_index != vulkan_context.graphics_compute_queue_index && vulkan_context.transfer_queue_index != vulkan_context.present_queue_index)
    {
        queue_create_info.queueFamilyIndex = vulkan_context.transfer_queue_index;
        queue_create_info.queueCount = 1;
        queue_create_infos[queue_info_count++] = queue_create_info;
    }

    vkGetPhysicalDeviceProperties(vulkan_context.physical_device, &vulkan_context.physical_device_properties);
    vkGetPhysicalDeviceMemoryProperties(vulkan_context.physical_device, &vulkan_context.physical_device_memory_properties);

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT physical_device_descriptor_indexing_features = {};
    physical_device_descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

    VkPhysicalDeviceFeatures2 physical_device_features2 = {};
    physical_device_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physical_device_features2.pNext = &physical_device_descriptor_indexing_features;
    vkGetPhysicalDeviceFeatures2KHR(vulkan_context.physical_device, &physical_device_features2);

    #if !defined(DEBUG)
        // NOTE: Vulkan spec says this is bounds checking and there may be a performance penalty for enabling this feature.
        // Only enable it in debug mode.
        physical_device_features2.features.robustBufferAccess = false;
    #endif

    // TODO: Assert that we have all the features we need
    vulkan_context.physical_device_features = physical_device_features2.features;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = &physical_device_features2;
    device_create_info.queueCreateInfoCount = queue_info_count;
    device_create_info.pQueueCreateInfos = &queue_create_infos[0];

    char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
    device_create_info.enabledExtensionCount = array_count(device_extensions);
    device_create_info.ppEnabledExtensionNames = &device_extensions[0];

    VK_CALL(vkCreateDevice(vulkan_context.physical_device, &device_create_info, NULL, &vulkan_context.device));

    #define VK_FUNCTION(function) function = (PFN_##function)vkGetDeviceProcAddr(vulkan_context.device, #function);
        VK_FUNCTIONS_DEVICE
    #undef VK_FUNCTION

    vkGetDeviceQueue(vulkan_context.device, vulkan_context.graphics_compute_queue_index, 0, &vulkan_context.graphics_compute_queue);
    vkGetDeviceQueue(vulkan_context.device, vulkan_context.present_queue_index, 0, &vulkan_context.present_queue);
    vkGetDeviceQueue(vulkan_context.device, vulkan_context.transfer_queue_index, (vulkan_context.transfer_queue_index == vulkan_context.graphics_compute_queue_index || vulkan_context.transfer_queue_index == vulkan_context.present_queue_index) && separate_transfer_queue ? 1 : 0, &vulkan_context.transfer_queue);

    recreate_swapchain(window_width, window_height);

    renderer.max_2d_textures = min(MAX_2D_TEXTURES, vulkan_context.physical_device_properties.limits.maxPerStageDescriptorSampledImages);

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = sampler_create_info.addressModeU;
    sampler_create_info.addressModeW = sampler_create_info.addressModeU;
    sampler_create_info.anisotropyEnable = vulkan_context.physical_device_features.samplerAnisotropy;
    sampler_create_info.maxAnisotropy = sampler_create_info.anisotropyEnable ? vulkan_context.physical_device_properties.limits.maxSamplerAnisotropy : 1.0f;
    sampler_create_info.maxLod = vulkan_context.physical_device_properties.limits.maxSamplerLodBias;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    VK_CALL(vkCreateSampler(vulkan_context.device, &sampler_create_info, NULL, &renderer.texture_2d_sampler));

    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = sampler_create_info.addressModeU;
    sampler_create_info.addressModeW = sampler_create_info.addressModeU;
    VK_CALL(vkCreateSampler(vulkan_context.device, &sampler_create_info, NULL, &renderer.voxel_texture_sampler));

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.queueFamilyIndex = vulkan_context.transfer_queue_index;
    VK_CALL(vkCreateCommandPool(vulkan_context.device, &command_pool_create_info, NULL, &renderer.transfer_command_pool));

    command_pool_create_info.queueFamilyIndex = vulkan_context.graphics_compute_queue_index;
    VK_CALL(vkCreateCommandPool(vulkan_context.device, &command_pool_create_info, NULL, &renderer.graphics_command_pool));

    VkDescriptorSetLayoutBinding global_layout_bindings[] =
    {
        {0, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, &renderer.texture_2d_sampler},
        {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, renderer.max_2d_textures, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
        {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT},

        {4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
        {5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT},

        {6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
        {7, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT},
        {8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT},

        {9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
        {10, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
        {11, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT}
    };

    VkDescriptorBindingFlagsEXT global_binding_flags[array_count(global_layout_bindings)] = {};
    global_binding_flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layout_binding_flags_create_info = {};
    layout_binding_flags_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    layout_binding_flags_create_info.bindingCount = array_count(global_binding_flags);
    layout_binding_flags_create_info.pBindingFlags = global_binding_flags;

    VkDescriptorSetLayoutCreateInfo global_layout_create_info = {};
    global_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    global_layout_create_info.pNext = &layout_binding_flags_create_info;
    global_layout_create_info.bindingCount = array_count(global_layout_bindings);
    global_layout_create_info.pBindings = global_layout_bindings;
    VK_CALL(vkCreateDescriptorSetLayout(vulkan_context.device, &global_layout_create_info, NULL, &renderer.global_descriptor_set_layout));

    VkDescriptorSetLayoutBinding per_frame_layout_bindings[] =
    {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT}
    };

    VkDescriptorSetLayoutCreateInfo per_frame_layout_create_info = {};
    per_frame_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    per_frame_layout_create_info.bindingCount = array_count(per_frame_layout_bindings);
    per_frame_layout_create_info.pBindings = per_frame_layout_bindings;
    VK_CALL(vkCreateDescriptorSetLayout(vulkan_context.device, &per_frame_layout_create_info, NULL, &renderer.per_frame_descriptor_set_layout));

    VkDescriptorPoolSize descriptor_pool_sizes[] =
    {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, renderer.max_2d_textures + 2},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAME_RESOURCES}
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.poolSizeCount = array_count(descriptor_pool_sizes);
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes;
    descriptor_pool_create_info.maxSets = 1 + MAX_FRAME_RESOURCES;
    VK_CALL(vkCreateDescriptorPool(vulkan_context.device, &descriptor_pool_create_info, NULL, &renderer.descriptor_pool));

    VkDescriptorSet descriptor_sets[1 + MAX_FRAME_RESOURCES];
    VkDescriptorSetLayout set_layouts[1 + MAX_FRAME_RESOURCES];
    set_layouts[MAX_FRAME_RESOURCES] = renderer.global_descriptor_set_layout;
    for (u32 i = 0; i < MAX_FRAME_RESOURCES; ++i)
    {
        set_layouts[i] = renderer.per_frame_descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool = renderer.descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = array_count(descriptor_sets);
    descriptor_set_allocate_info.pSetLayouts = set_layouts;
    VK_CALL(vkAllocateDescriptorSets(vulkan_context.device, &descriptor_set_allocate_info, descriptor_sets));

    renderer.global_descriptor_set = descriptor_sets[MAX_FRAME_RESOURCES];
    for (u32 i = 0; i < MAX_FRAME_RESOURCES; ++i)
    {
        renderer.frame_resources[i].descriptor_set = descriptor_sets[i];
    }

    // TODO: Pass in as a renderer setting?
    recreate_main_framebuffer({1920, 1080});
    recreate_cluster_grid({16, 9, 24});
    recreate_voxel_grid(256);

    VkDescriptorSetLayout layouts[] = {renderer.per_frame_descriptor_set_layout, renderer.global_descriptor_set_layout};
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = array_count(layouts);
    pipeline_layout_create_info.pSetLayouts = layouts;
    VK_CALL(vkCreatePipelineLayout(vulkan_context.device, &pipeline_layout_create_info, NULL, &renderer.global_pipeline_layout));

    // NOTE: Ignoring pipeline cache for now
    VK_CALL(vulkan_create_clusterizer_pipeline(vulkan_context.device, renderer.global_pipeline_layout, &renderer.clusterizer_pipeline));
    VK_CALL(vulkan_create_cluster_light_culling_pipeline(vulkan_context.device, renderer.global_pipeline_layout, &renderer.cluster_light_culling_pipeline));
    VK_CALL(vulkan_create_main_pipeline(vulkan_context.device, renderer.global_pipeline_layout, renderer.main_render_pass, 0, &renderer.main_pipeline));
    VK_CALL(vulkan_create_final_pass_pipeline(vulkan_context.device, renderer.global_pipeline_layout, vulkan_context.swapchain_render_pass, 0, &renderer.final_pass_pipeline));
    VK_CALL(vulkan_create_voxelizer_pipeline(vulkan_context.device, renderer.global_pipeline_layout, renderer.voxelization_render_pass, 0, &renderer.voxelizer_pipeline));
    VK_CALL(vulkan_create_voxelizer_compute_pipeline(vulkan_context.device, renderer.global_pipeline_layout, &renderer.voxelizer_compute_pipeline));
    VK_CALL(vulkan_create_debug_voxel_visualizer_pipeline(vulkan_context.device, renderer.global_pipeline_layout, renderer.main_render_pass, 0, &renderer.DEBUG_voxel_visualizer_pipeline));
    VK_CALL(vulkan_create_debug_cluster_visualizer_pipeline(vulkan_context.device, renderer.global_pipeline_layout, renderer.main_render_pass, 0, &renderer.DEBUG_cluster_visualizer_pipeline));

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = TRANSFER_MEMORY_SIZE;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer.transfer_buffer, &renderer.transfer_memory_block);

    renderer.transfer_queue = {renderer.transfer_memory_block.base, renderer.transfer_memory_block.size};

    u64 aligned_frame_uniforms_offset = ((DRAW_CALLS_MEMORY_SIZE + DRAW_INSTANCE_MEMORY_SIZE) + vulkan_context.physical_device_properties.limits.minUniformBufferOffsetAlignment - 1) & -(s64)vulkan_context.physical_device_properties.limits.minUniformBufferOffsetAlignment;
    u64 aligned_light_storage_offset = ((aligned_frame_uniforms_offset + FRAME_UNIFORMS_MEMORY_SIZE) + vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment - 1) & -(s64)vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment;
    u64 aligned_frame_data_size = ((aligned_light_storage_offset + LIGHT_STORAGE_MEMORY_SIZE) + vulkan_context.physical_device_properties.limits.minUniformBufferOffsetAlignment - 1) & -(s64)vulkan_context.physical_device_properties.limits.minUniformBufferOffsetAlignment;
    buffer_create_info.size = aligned_frame_data_size * MAX_FRAME_RESOURCES;
    buffer_create_info.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer.frame_resources_buffer, &renderer.frame_resources_memory_block);

    renderer.mesh_buffer_used = 0;
    buffer_create_info.size = MESH_MEMORY_SIZE;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer.mesh_buffer, &renderer.mesh_memory_block);

    renderer.storage_materials_offset = 0;
    renderer.storage_xforms_offset = (MATERIALS_MEMORY_SIZE + vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment - 1) & -(s64)vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment;
    u64 aligned_storage_size = ((renderer.storage_xforms_offset + XFORMS_MEMORY_SIZE) + vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment - 1) & -(s64)vulkan_context.physical_device_properties.limits.minStorageBufferOffsetAlignment;
    buffer_create_info.size = aligned_storage_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    allocate_vulkan_buffer(&buffer_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer.storage_buffer, &renderer.storage_memory_block);

    VkDescriptorBufferInfo material_buffer_info = {renderer.storage_buffer, renderer.storage_materials_offset, MATERIALS_MEMORY_SIZE};
    VkWriteDescriptorSet material_storage_write = {};
    material_storage_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    material_storage_write.dstSet = renderer.global_descriptor_set;
    material_storage_write.dstBinding = 2;
    material_storage_write.descriptorCount = 1;
    material_storage_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    material_storage_write.pBufferInfo = &material_buffer_info;

    VkDescriptorBufferInfo xform_buffer_info = {renderer.storage_buffer, renderer.storage_xforms_offset, XFORMS_MEMORY_SIZE};
    VkWriteDescriptorSet xform_storage_write = material_storage_write;
    xform_storage_write.dstBinding = 3;
    xform_storage_write.pBufferInfo = &xform_buffer_info;

    VkWriteDescriptorSet storage_writes[] = {material_storage_write, xform_storage_write};
    vkUpdateDescriptorSets(vulkan_context.device, array_count(storage_writes), storage_writes, 0, NULL);

    // NOTE: Creating a dummy image to get memory type bits in order to allocate texture memory.
    // All 2D textures that will need to use this memory should have the same memory requirements.
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_BC7_SRGB_BLOCK;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage dummy_image;
    VK_CALL(vkCreateImage(vulkan_context.device, &image_create_info, NULL, &dummy_image));
    VkMemoryRequirements image_memory_requirements;
    vkGetImageMemoryRequirements(vulkan_context.device, dummy_image, &image_memory_requirements);

    renderer.texture_memory_block = allocate_vulkan_memory_block(TEXTURE_MEMORY_SIZE, get_memory_type_index(image_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    vkDestroyImage(vulkan_context.device, dummy_image, NULL);

    // TODO: How many buffer slots should be reserved?
    renderer.mesh_buffers = memory_arena_reserve_array(vulkan_context.memory_arena, Vulkan_Buffer, 4096);
    renderer.textures_2d = memory_arena_reserve_array(vulkan_context.memory_arena, Vulkan_Texture, renderer.max_2d_textures);

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CALL(vkCreateFence(vulkan_context.device, &fence_create_info, NULL, &renderer.transfer_command_buffer_fence));

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = renderer.transfer_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    VK_CALL(vkAllocateCommandBuffers(vulkan_context.device, &command_buffer_allocate_info, &renderer.transfer_command_buffer));

    command_buffer_allocate_info.commandPool = renderer.graphics_command_pool;

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (u32 i = 0; i < MAX_FRAME_RESOURCES; ++i)
    {
        VK_CALL(vkCreateSemaphore(vulkan_context.device, &semaphore_create_info, NULL, &renderer.frame_resources[i].image_available_semaphore));
        VK_CALL(vkCreateSemaphore(vulkan_context.device, &semaphore_create_info, NULL, &renderer.frame_resources[i].render_finished_semaphore));
        VK_CALL(vkCreateFence(vulkan_context.device, &fence_create_info, NULL, &renderer.frame_resources[i].graphics_compute_command_buffer_fence));
        VK_CALL(vkAllocateCommandBuffers(vulkan_context.device, &command_buffer_allocate_info, &renderer.frame_resources[i].graphics_compute_command_buffer));

        renderer.frame_resources[i].should_record_command_buffer = true;

        // NOTE: Layout of frame_resources_buffer is [DRAW_INDIRECT_COUNT | DRAW_INDIRECT_COMMANDS | DRAW_INSTANCE_DATA | FRAME_UNIFORM_DATA | LIGHT_DATA] * MAX_FRAME_RESOURCES
        renderer.frame_resources[i].data_memory_size = aligned_frame_data_size;
        renderer.frame_resources[i].draw_call_count_offset = i * renderer.frame_resources[i].data_memory_size;
        renderer.frame_resources[i].draw_command_offset = renderer.frame_resources[i].draw_call_count_offset + sizeof(u32);
        renderer.frame_resources[i].draw_instance_offset = renderer.frame_resources[i].draw_command_offset + DRAW_CALLS_MEMORY_SIZE - sizeof(u32);
        renderer.frame_resources[i].uniforms_offset = renderer.frame_resources[i].draw_instance_offset + (aligned_frame_uniforms_offset - DRAW_CALLS_MEMORY_SIZE);
        renderer.frame_resources[i].light_storage_offset = renderer.frame_resources[i].uniforms_offset + (aligned_light_storage_offset - aligned_frame_uniforms_offset);

        renderer.frame_resources[i].draw_call_count = (u32*)(renderer.frame_resources_memory_block.base + renderer.frame_resources[i].draw_call_count_offset);
        renderer.frame_resources[i].draw_command_arena = {renderer.frame_resources_memory_block.base + renderer.frame_resources[i].draw_command_offset, DRAW_CALLS_MEMORY_SIZE - sizeof(u32)};
        renderer.frame_resources[i].draw_instance_arena = {renderer.frame_resources_memory_block.base + renderer.frame_resources[i].draw_instance_offset, DRAW_INSTANCE_MEMORY_SIZE};
        renderer.frame_resources[i].uniforms = (Frame_Uniforms*)(renderer.frame_resources_memory_block.base + renderer.frame_resources[i].uniforms_offset);
        renderer.frame_resources[i].light_arena = {renderer.frame_resources_memory_block.base + renderer.frame_resources[i].light_storage_offset, LIGHT_STORAGE_MEMORY_SIZE};

        VkDescriptorBufferInfo uniforms_buffer_info = {renderer.frame_resources_buffer, renderer.frame_resources[i].uniforms_offset, sizeof(Frame_Uniforms)};
        VkWriteDescriptorSet uniform_write = {};
        uniform_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniform_write.dstSet = renderer.frame_resources[i].descriptor_set;
        uniform_write.dstBinding = 0;
        uniform_write.descriptorCount = 1;
        uniform_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_write.pBufferInfo = &uniforms_buffer_info;

        VkDescriptorBufferInfo light_storage_info = {renderer.frame_resources_buffer, renderer.frame_resources[i].light_storage_offset, LIGHT_STORAGE_MEMORY_SIZE};
        VkWriteDescriptorSet storage_write = uniform_write;
        storage_write.dstBinding = 1;
        storage_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storage_write.pBufferInfo = &light_storage_info;

        VkWriteDescriptorSet descriptor_writes[] = {uniform_write, storage_write};
        vkUpdateDescriptorSets(vulkan_context.device, array_count(descriptor_writes), descriptor_writes, 0, NULL);
    }
}

struct Acquire_Next_Image_Job_Data
{
    VkSemaphore image_available_semaphore;
    u32*        swapchain_image_index;
    VkResult*   result;
};

struct Wait_For_Fences_Job_Data
{
    VkFence*  fences;
    u32       fence_count;
    VkResult* result;
};

JOB_ENTRY_POINT(acquire_next_image_entry_point)
{
    Acquire_Next_Image_Job_Data* job_data = (Acquire_Next_Image_Job_Data*)data;
    *job_data->result = vkAcquireNextImageKHR(vulkan_context.device, vulkan_context.swapchain, U64_MAX, job_data->image_available_semaphore, NULL, job_data->swapchain_image_index);
}

JOB_ENTRY_POINT(wait_for_fences_entry_point)
{
    Wait_For_Fences_Job_Data* job_data = (Wait_For_Fences_Job_Data*)data;
    *job_data->result = vkWaitForFences(vulkan_context.device, job_data->fence_count, job_data->fences, true, U64_MAX);
}

internal VkResult acquire_next_image(VkSemaphore image_available_semaphore, u32* swapchain_image_index)
{
    VkResult result;
    Job_Counter counter;

    Acquire_Next_Image_Job_Data job_data = {image_available_semaphore, swapchain_image_index, &result};
    Job job = {acquire_next_image_entry_point, &job_data, NULL};

    run_jobs_on_dedicated_thread(&job, 1, &counter);
    wait_for_counter(&counter, 0);
    return result;
}

internal VkResult wait_for_fences(VkFence* fences, u32 fence_count)
{
    VkResult result;
    Job_Counter counter;

    Wait_For_Fences_Job_Data job_data = {fences, fence_count, &result};
    Job job = {wait_for_fences_entry_point, &job_data, NULL};

    run_jobs_on_dedicated_thread(&job, 1, &counter);
    wait_for_counter(&counter, 0);
    return result;
}

Renderer_Transfer_Operation* renderer_request_transfer(Renderer_Transfer_Operation_Type type)
{
    assert(type == RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL || type == RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM);
    return renderer_request_transfer(type, type == RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL ? sizeof(Material) : sizeof(m4x4));
}

Renderer_Transfer_Operation* renderer_request_transfer(Renderer_Transfer_Operation_Type type, u64 transfer_size)
{
    Renderer_Transfer_Operation* operation = NULL;
    if (renderer.transfer_queue.operation_count < array_count(renderer.transfer_queue.operations) && renderer.transfer_queue.transfer_memory_used + transfer_size <= renderer.transfer_queue.transfer_memory_size)
    {
        u64 size = transfer_size;
        u64 aligned_offset = type == RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE ? ((renderer.transfer_queue.enqueue_location + 16 - 1) & -16) - renderer.transfer_queue.enqueue_location : 0;
        if (renderer.transfer_queue.enqueue_location + size + aligned_offset >= renderer.transfer_queue.transfer_memory_size)
        {
            if (transfer_size < renderer.transfer_queue.dequeue_location)
            {
                size += renderer.transfer_queue.transfer_memory_size - renderer.transfer_queue.enqueue_location;
                renderer.transfer_queue.enqueue_location = 0;
                aligned_offset = 0;
            }
            else
            {
                return NULL;
            }
        }

        operation = &renderer.transfer_queue.operations[(renderer.transfer_queue.operation_index++ % array_count(renderer.transfer_queue.operations))];
        assert(operation->state == RENDERER_TRANSFER_OPERATION_STATE_UNLOADED || operation->state == RENDERER_TRANSFER_OPERATION_STATE_FINISHED);

        atomic_increment(&renderer.transfer_queue.operation_count);

        operation->memory = renderer.transfer_queue.transfer_memory + renderer.transfer_queue.enqueue_location + aligned_offset;
        operation->size = size;
        operation->state = RENDERER_TRANSFER_OPERATION_STATE_UNLOADED;
        operation->type = type;
        operation->counter = NULL;

        renderer.transfer_queue.enqueue_location = (renderer.transfer_queue.enqueue_location + transfer_size + aligned_offset) % renderer.transfer_queue.transfer_memory_size;
        atomic_add((s64*)&renderer.transfer_queue.transfer_memory_used, size + aligned_offset);

        assert(renderer.transfer_queue.transfer_memory_used <= renderer.transfer_queue.transfer_memory_size);
    }

    return operation;
}

void renderer_queue_transfer(Renderer_Transfer_Operation* operation, Job_Counter* counter)
{
    operation->state = RENDERER_TRANSFER_OPERATION_STATE_READY;
    if (counter)
    {
        atomic_increment(counter);
        operation->counter = counter;
    }
}

internal void resolve_pending_transfer_operations()
{
    if (vkGetFenceStatus(vulkan_context.device, renderer.transfer_command_buffer_fence) == VK_SUCCESS && renderer.transfer_queue.operation_count > 0)
    {
        u64 transfer_operations = renderer.transfer_queue.operation_count;
        u64 starting_index = ((renderer.transfer_queue.operation_index - transfer_operations) % array_count(renderer.transfer_queue.operations) + array_count(renderer.transfer_queue.operations)) % array_count(renderer.transfer_queue.operations);

        Memory_Arena_Marker memory_arena_marker = memory_arena_get_marker(vulkan_context.memory_arena);

        VkBufferCopy* mesh_buffer_copies = memory_arena_reserve_array(vulkan_context.memory_arena, VkBufferCopy, transfer_operations);
        u32 mesh_copy_count = 0;
        u64 mesh_buffer_dst_offset = renderer.mesh_buffer_used;
        b32 create_new_mesh_buffer_copy = true;

        VkBufferCopy* material_copies = memory_arena_reserve_array(vulkan_context.memory_arena, VkBufferCopy, transfer_operations);
        u64 material_copy_count = 0;
        s32 last_material_index = S32_MIN;

        VkBufferCopy* xform_copies = memory_arena_reserve_array(vulkan_context.memory_arena, VkBufferCopy, transfer_operations);
        u64 xform_copy_count = 0;
        s32 last_xform_index = S32_MIN;

        // TODO: How many VkBufferImageCopies should we actually get?
        VkBufferImageCopy* texture_copies = memory_arena_reserve_array(vulkan_context.memory_arena, VkBufferImageCopy, transfer_operations * 16);
        u32 texture_copy_count = 0;

        VkImageMemoryBarrier* texture_barriers = memory_arena_reserve_array(vulkan_context.memory_arena, VkImageMemoryBarrier, transfer_operations);
        u32 texture_barrier_count = 0;

        for (u64 i = 0; i < transfer_operations; ++i)
        {
            Renderer_Transfer_Operation* operation = &renderer.transfer_queue.operations[(starting_index + i) % array_count(renderer.transfer_queue.operations)];
            if (operation->state == RENDERER_TRANSFER_OPERATION_STATE_QUEUED)
            {
                operation->state = RENDERER_TRANSFER_OPERATION_STATE_FINISHED;
                atomic_decrement(&renderer.transfer_queue.operation_count);
                if (operation->counter)
                {
                    atomic_decrement(operation->counter);
                }
            }
            else if (operation->state != RENDERER_TRANSFER_OPERATION_STATE_READY)
            {
                break;
            }
            else
            {
                u64 aligned_offset = operation->type == RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE ? ((renderer.transfer_queue.dequeue_location + 16 - 1) & -16) - renderer.transfer_queue.dequeue_location : 0;
                u64 src_offset = renderer.transfer_queue.dequeue_location + aligned_offset;
                u64 size = operation->size;
                if (renderer.transfer_queue.dequeue_location + operation->size + aligned_offset >= renderer.transfer_queue.transfer_memory_size)
                {
                    size = operation->size - renderer.transfer_queue.transfer_memory_size - renderer.transfer_queue.dequeue_location;
                    src_offset = 0;
                    aligned_offset = 0;
                    create_new_mesh_buffer_copy = true;
                    last_material_index = S32_MIN;
                    last_xform_index = S32_MIN;
                }

                switch (operation->type)
                {
                case RENDERER_TRANSFER_OPERATION_TYPE_MESH_BUFFER:
                {
                    if (create_new_mesh_buffer_copy)
                    {
                        if (mesh_copy_count > 0)
                        {
                            mesh_buffer_dst_offset += mesh_buffer_copies[mesh_copy_count - 1].size;
                        }
                        mesh_buffer_copies[mesh_copy_count].srcOffset = src_offset;
                        mesh_buffer_copies[mesh_copy_count].dstOffset = mesh_buffer_dst_offset;
                        mesh_buffer_copies[mesh_copy_count].size = 0;
                        mesh_copy_count++;

                        create_new_mesh_buffer_copy = false;
                    }

                    VkBufferCopy* buffer_copy = &mesh_buffer_copies[mesh_copy_count - 1];
                    buffer_copy->size += size;

                    // TODO: Reuse buffer slot. We also need to free the buffer at this slot if it was previously used.
                    renderer.mesh_buffers[operation->buffer.id].offset = (u32)renderer.mesh_buffer_used;
                    renderer.mesh_buffer_used += size;
                    last_material_index = S32_MIN;
                    last_xform_index = S32_MIN;
                } break;

                case RENDERER_TRANSFER_OPERATION_TYPE_MATERIAL:
                {
                    assert(size == sizeof(Material));
                    if (last_material_index + 1 != operation->material.id)
                    {
                        material_copies[material_copy_count].srcOffset = src_offset;
                        material_copies[material_copy_count].dstOffset = renderer.storage_materials_offset + operation->material.id * size;
                        material_copies[material_copy_count].size = 0;
                        material_copy_count++;

                        last_material_index = operation->material.id;
                    }

                    VkBufferCopy* material_copy = &material_copies[material_copy_count - 1];
                    material_copy->size += size;

                    create_new_mesh_buffer_copy = true;
                    last_xform_index = S32_MIN;
                } break;

                case RENDERER_TRANSFER_OPERATION_TYPE_TRANSFORM:
                {
                    assert(size == sizeof(m4x4));
                    if (last_xform_index + 1 != operation->transform.id)
                    {
                        xform_copies[xform_copy_count].srcOffset = src_offset;
                        xform_copies[xform_copy_count].dstOffset = renderer.storage_xforms_offset + operation->transform.id * size;
                        xform_copies[xform_copy_count].size = 0;
                        xform_copy_count++;

                        last_xform_index = operation->transform.id;
                    }

                    VkBufferCopy* xform_copy = &xform_copies[xform_copy_count - 1];
                    xform_copy->size += size;

                    create_new_mesh_buffer_copy = true;
                    last_material_index = S32_MIN;
                } break;

                case RENDERER_TRANSFER_OPERATION_TYPE_TEXTURE:
                {
                    VkImageCreateInfo image_create_info = {};
                    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                    image_create_info.imageType = VK_IMAGE_TYPE_2D;
                    image_create_info.format = (VkFormat)operation->texture.format;
                    image_create_info.extent = {operation->texture.width, operation->texture.height, 1};
                    image_create_info.mipLevels = operation->texture.mipmap_count;
                    image_create_info.arrayLayers = 1;
                    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
                    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
                    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

                    assert(image_create_info.format == VK_FORMAT_BC7_SRGB_BLOCK || image_create_info.format == VK_FORMAT_BC7_UNORM_BLOCK || image_create_info.format == VK_FORMAT_BC5_UNORM_BLOCK);

                    // TODO: Reuse texture slot. We also need to free the textures at this slot if it was previously used.
                    // Also vulkan_memory_block_reserve_image just reserves image memory from where renderer.texture_memory_used is.
                    // We need to allocate more smartly.
                    Vulkan_Texture* texture = &renderer.textures_2d[operation->texture.id];
                    texture->image = vulkan_memory_block_reserve_image(&renderer.texture_memory_block, &image_create_info);

                    VkImageViewCreateInfo image_view_create_info = {};
                    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    image_view_create_info.image = texture->image;
                    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    image_view_create_info.format = image_create_info.format;
                    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, image_create_info.mipLevels, 0, image_create_info.arrayLayers};
                    VK_CALL(vkCreateImageView(vulkan_context.device, &image_view_create_info, NULL, &texture->image_view));

                    VkImageMemoryBarrier* image_barrier = &texture_barriers[texture_barrier_count++];
                    *image_barrier = {};
                    image_barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    image_barrier->srcAccessMask = 0;
                    image_barrier->dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    image_barrier->oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    image_barrier->newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    image_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    image_barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    image_barrier->image = texture->image;
                    image_barrier->subresourceRange = image_view_create_info.subresourceRange;

                    u32 width = image_create_info.extent.width;
                    u32 height = image_create_info.extent.height;
                    u64 buffer_offset = src_offset;

                    for (u32 i = 0; i < image_create_info.mipLevels; ++i)
                    {
                        VkBufferImageCopy* buffer_image_copy_region = &texture_copies[texture_copy_count++];
                        *buffer_image_copy_region = {};
                        buffer_image_copy_region->imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, image_create_info.arrayLayers};
                        buffer_image_copy_region->imageExtent = {width, height, 1};
                        buffer_image_copy_region->bufferOffset = buffer_offset;

                        // NOTE: This calculates size of a mip level for the BC5/BC7 format
                        buffer_offset += max(1, (width + 3) / 4) * 16 * max(1, (height + 3) / 4);
                        width = max(1, width >> 1);
                        height = max(1, height >> 1);
                    }

                    VkDescriptorImageInfo image_info = {NULL, texture->image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                    VkWriteDescriptorSet texture_write = {};
                    texture_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    texture_write.dstSet = renderer.global_descriptor_set;
                    texture_write.dstBinding = 1;
                    texture_write.dstArrayElement = operation->texture.id;
                    texture_write.descriptorCount = 1;
                    texture_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    texture_write.pImageInfo = &image_info;
                    vkUpdateDescriptorSets(vulkan_context.device, 1, &texture_write, 0, NULL);

                    renderer.texture_memory_used += buffer_offset - src_offset;

                    create_new_mesh_buffer_copy = true;
                    last_material_index = S32_MIN;
                    last_xform_index = S32_MIN;
                } break;

                default:
                    invalid_code_path();
                    break;
                }

                operation->state = RENDERER_TRANSFER_OPERATION_STATE_QUEUED;

                renderer.transfer_queue.dequeue_location = (renderer.transfer_queue.dequeue_location + operation->size + aligned_offset) % renderer.transfer_queue.transfer_memory_size;
                atomic_add((s64*)&renderer.transfer_queue.transfer_memory_used, -(s64)(operation->size + aligned_offset));
            }
        }

        if (mesh_copy_count > 0 || texture_barrier_count > 0 || material_copy_count > 0 || xform_copy_count)
        {
            vkResetFences(vulkan_context.device, 1, &renderer.transfer_command_buffer_fence);
            vkResetCommandPool(vulkan_context.device, renderer.transfer_command_pool, 0);

            VkCommandBufferBeginInfo command_buffer_begin_info = {};
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(renderer.transfer_command_buffer, &command_buffer_begin_info);

            if (mesh_copy_count > 0)
            {
                vkCmdCopyBuffer(renderer.transfer_command_buffer, renderer.transfer_buffer, renderer.mesh_buffer, mesh_copy_count, mesh_buffer_copies);
            }
            if (material_copy_count > 0)
            {
                vkCmdCopyBuffer(renderer.transfer_command_buffer, renderer.transfer_buffer, renderer.storage_buffer, material_copy_count, material_copies);
            }
            if (xform_copy_count > 0)
            {
                vkCmdCopyBuffer(renderer.transfer_command_buffer, renderer.transfer_buffer, renderer.storage_buffer, xform_copy_count, xform_copies);
            }

            VkBufferMemoryBarrier* buffer_barriers = NULL;
            if (mesh_copy_count > 0 || material_copy_count > 0 || xform_copy_count > 0)
            {
                buffer_barriers = memory_arena_reserve_array(vulkan_context.memory_arena, VkBufferMemoryBarrier, mesh_copy_count + material_copy_count + xform_copy_count);

                u32 i = 0;
                for (; i < mesh_copy_count; ++i)
                {
                    buffer_barriers[i] = {};
                    buffer_barriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    buffer_barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    buffer_barriers[i].dstAccessMask = 0;
                    buffer_barriers[i].srcQueueFamilyIndex = vulkan_context.transfer_queue_index;
                    buffer_barriers[i].dstQueueFamilyIndex = vulkan_context.graphics_compute_queue_index;
                    buffer_barriers[i].buffer = renderer.mesh_buffer;
                    buffer_barriers[i].offset = mesh_buffer_copies[i].dstOffset;
                    buffer_barriers[i].size = mesh_buffer_copies[i].size;
                }
                for (; i < mesh_copy_count + material_copy_count; ++i)
                {
                    buffer_barriers[i] = {};
                    buffer_barriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    buffer_barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    buffer_barriers[i].dstAccessMask = 0;
                    buffer_barriers[i].srcQueueFamilyIndex = vulkan_context.transfer_queue_index;
                    buffer_barriers[i].dstQueueFamilyIndex = vulkan_context.graphics_compute_queue_index;
                    buffer_barriers[i].buffer = renderer.storage_buffer;
                    buffer_barriers[i].offset = material_copies[i - mesh_copy_count].dstOffset;
                    buffer_barriers[i].size = material_copies[i - mesh_copy_count].size;
                }
                for (; i < mesh_copy_count + material_copy_count + xform_copy_count; ++i)
                {
                    buffer_barriers[i] = {};
                    buffer_barriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    buffer_barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    buffer_barriers[i].dstAccessMask = 0;
                    buffer_barriers[i].srcQueueFamilyIndex = vulkan_context.transfer_queue_index;
                    buffer_barriers[i].dstQueueFamilyIndex = vulkan_context.graphics_compute_queue_index;
                    buffer_barriers[i].buffer = renderer.storage_buffer;
                    buffer_barriers[i].offset = xform_copies[i - mesh_copy_count - material_copy_count].dstOffset;
                    buffer_barriers[i].size = xform_copies[i - mesh_copy_count - material_copy_count].size;
                }
            }

            if (texture_barrier_count > 0)
            {
                vkCmdPipelineBarrier(renderer.transfer_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, texture_barrier_count, texture_barriers);

                u32 texture_copy_index = 0;
                for (u32 i = 0; i < texture_barrier_count; ++i)
                {
                    assert(texture_copy_index < texture_copy_count);

                    vkCmdCopyBufferToImage(renderer.transfer_command_buffer, renderer.transfer_buffer, texture_barriers[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture_barriers[i].subresourceRange.levelCount, &texture_copies[texture_copy_index]);
                    texture_copy_index += texture_barriers[i].subresourceRange.levelCount;

                    texture_barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    texture_barriers[i].dstAccessMask = 0;
                    texture_barriers[i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    texture_barriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    texture_barriers[i].srcQueueFamilyIndex = vulkan_context.transfer_queue_index;
                    texture_barriers[i].dstQueueFamilyIndex = vulkan_context.graphics_compute_queue_index;
                }
            }

            vkCmdPipelineBarrier(renderer.transfer_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, mesh_copy_count + material_copy_count + xform_copy_count, buffer_barriers, texture_barrier_count, texture_barriers);

            vkEndCommandBuffer(renderer.transfer_command_buffer);

            VkSubmitInfo submit_info = {};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &renderer.transfer_command_buffer;
            vkQueueSubmit(vulkan_context.transfer_queue, 1, &submit_info, renderer.transfer_command_buffer_fence);
        }

        memory_arena_free_to_marker(vulkan_context.memory_arena, memory_arena_marker);
    }
}

internal VkRect2D aspect_ratio_corrected_render_area(u32 render_width, u32 render_height, u32 window_width, u32 window_height)
{
    VkRect2D result = {};
    if (render_width > 0 && render_height > 0 && window_width > 0 && window_height > 0)
    {
        f32 width = (f32)window_height * (f32)render_width / render_height;
        f32 height = (f32)window_width * (f32)render_height / render_width;

        if (width >= (f32)window_width)
        {
            result.offset.x = 0;
            result.offset.y = (u32)((window_height - height) / 2.0f);
            result.extent.width = window_width;
            result.extent.height = (u32)height;
        }
        else
        {
            result.offset.x = (u32)((window_width - width) / 2.0f);
            result.offset.y = 0;
            result.extent.width = (u32)width;
            result.extent.height = window_height;
        }
    }
    return result;
}

void renderer_begin_frame(Frame_Parameters* frame_params)
{
    renderer.current_render_frame = &renderer.frame_resources[frame_params->frame_number % MAX_FRAME_RESOURCES];
    *renderer.current_render_frame->draw_call_count = 0;
    memory_arena_reset(&renderer.current_render_frame->draw_command_arena);

    renderer.current_render_frame->draw_instance_count = 0;
    memory_arena_reset(&renderer.current_render_frame->draw_instance_arena);

    renderer.current_render_frame->uniforms->light_count = 0;
    memory_arena_reset(&renderer.current_render_frame->light_arena);

    renderer.current_render_frame->uniforms->view = frame_params->camera.view;
    renderer.current_render_frame->uniforms->projection = frame_params->camera.projection;
    renderer.current_render_frame->uniforms->view_projection = frame_params->camera.projection * renderer.current_render_frame->uniforms->view;
    renderer.current_render_frame->uniforms->inverse_projection = inverse(renderer.current_render_frame->uniforms->projection);
    renderer.current_render_frame->uniforms->inverse_view_projection = inverse(renderer.current_render_frame->uniforms->view_projection);

    renderer.current_render_frame->uniforms->DEBUG_view = frame_params->DEBUG_camera.view;

    renderer.current_render_frame->uniforms->inverse_render_dimensions = {1.0f / (f32)renderer.render_dimensions.x, 1.0f / (f32)renderer.render_dimensions.y};

    assert(renderer.voxel_grid_resolution.x == renderer.voxel_grid_resolution.y && renderer.voxel_grid_resolution.x == renderer.voxel_grid_resolution.z);
    renderer.current_render_frame->uniforms->voxel_grid_origin_resolution = {frame_params->camera.position.x, frame_params->camera.position.y, frame_params->camera.position.z, (f32)renderer.voxel_grid_resolution.x};
    renderer.current_render_frame->uniforms->voxel_size = 0.1;
    renderer.current_render_frame->uniforms->voxel_ray_step_size = 0.5;

    renderer.current_render_frame->uniforms->cluster_grid_resolution_size = {renderer.cluster_grid_resolution.x, renderer.cluster_grid_resolution.y, renderer.cluster_grid_resolution.z, (u32)(round_up(renderer.render_dimensions.x / renderer.cluster_grid_resolution.x))};
    renderer.current_render_frame->uniforms->camera_position = {frame_params->camera.position.x, frame_params->camera.position.y, frame_params->camera.position.z};
    renderer.current_render_frame->uniforms->camera_near = 0.1;
    renderer.current_render_frame->uniforms->camera_far = 100;
}

void renderer_draw_buffer(Renderer_Buffer buffer, u32 index_offset, u32 index_count, Renderer_Buffer material, Renderer_Buffer xform)
{
    renderer_draw_buffer(buffer, index_offset, index_count, 1, &material, &xform);
}

void renderer_draw_buffer(Renderer_Buffer buffer, u32 index_offset, u32 index_count, u32 instance_count, Renderer_Material* materials, Renderer_Transform* xforms)
{
    assert(renderer.current_render_frame);
    assert(*renderer.current_render_frame->draw_call_count < MAX_DRAW_CALLS_PER_FRAME);
    assert(renderer.current_render_frame->draw_instance_count < MAX_INSTANCES_PER_FRAME);

    (*renderer.current_render_frame->draw_call_count)++;

    // NOTE: The vertices and indices are in the same buffer.
    // We must pad the mesh's size (vertices + indices) to be a multiple of sizeof(Vertex) otherwise we wouldn't know what the correct vertex offset is!
    // We can get rid of this limitation if we separate the vertex and index buffer.
    VkDrawIndexedIndirectCommand* draw_command = (VkDrawIndexedIndirectCommand*)memory_arena_reserve(&renderer.current_render_frame->draw_command_arena, sizeof(VkDrawIndexedIndirectCommand));
    draw_command->indexCount = index_count;
    draw_command->instanceCount = instance_count;
    draw_command->vertexOffset = renderer.mesh_buffers[buffer.id].offset / sizeof(Vertex);
    draw_command->firstIndex = (renderer.mesh_buffers[buffer.id].offset + index_offset) / sizeof(u32);
    draw_command->firstInstance = renderer.current_render_frame->draw_instance_count;

    for (u32 i = 0; i < instance_count; ++i)
    {
        Draw_Instance_Data* instance_data = (Draw_Instance_Data*)memory_arena_reserve(&renderer.current_render_frame->draw_instance_arena, sizeof(Draw_Instance_Data));
        instance_data->xform_index = xforms[i].id;
        instance_data->material_index = materials[i].id;
    }
    renderer.current_render_frame->draw_instance_count += instance_count;
}

void renderer_draw_light(Light light)
{
    assert(renderer.current_render_frame);
    assert(renderer.current_render_frame->uniforms->light_count < MAX_LIGHTS);

    Light* light_data = (Light*)memory_arena_reserve(&renderer.current_render_frame->light_arena, sizeof(Light));
    copy_memory(light_data, &light, sizeof(Light));

    renderer.current_render_frame->uniforms->light_count++;
}

void renderer_end_frame()
{
    assert(renderer.current_render_frame);
}

void renderer_submit_frame(Frame_Parameters* frame_params)
{
    Frame_Resources* frame_resources = &renderer.frame_resources[frame_params->frame_number % MAX_FRAME_RESOURCES];

    // TODO: Is this the best place to resolve the transfer operations?
    resolve_pending_transfer_operations();

    wait_for_fences(&frame_resources->graphics_compute_command_buffer_fence, 1);
    vkResetFences(vulkan_context.device, 1, &frame_resources->graphics_compute_command_buffer_fence);

    if (frame_resources->should_record_command_buffer)
    {
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        VK_CALL(vkBeginCommandBuffer(frame_resources->graphics_compute_command_buffer, &command_buffer_begin_info));

        VkClearValue clear_values[2] = {};
        clear_values[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
        clear_values[1].depthStencil = {0.0f, 0};

        VkDescriptorSet descriptor_sets[] = {frame_resources->descriptor_set, renderer.global_descriptor_set};
        vkCmdBindDescriptorSets(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.global_pipeline_layout, 0, array_count(descriptor_sets), descriptor_sets, 0, NULL);
        vkCmdBindDescriptorSets(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, renderer.global_pipeline_layout, 0, array_count(descriptor_sets), descriptor_sets, 0, NULL);

        VkDeviceSize offsets[] = {frame_resources->draw_instance_offset, 0};
        VkBuffer buffers[] = {renderer.frame_resources_buffer, renderer.mesh_buffer};

        vkCmdBindVertexBuffers(frame_resources->graphics_compute_command_buffer, 0, array_count(buffers), buffers, offsets);
        vkCmdBindIndexBuffer(frame_resources->graphics_compute_command_buffer, renderer.mesh_buffer, 0, VK_INDEX_TYPE_UINT32);

        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

        // TODO: We don't need to do the clusterization pass every frame, only when the camera parameters change!
        // NOTE: Clusterization pass
        {
            vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, renderer.clusterizer_pipeline);
            vkCmdDispatch(frame_resources->graphics_compute_command_buffer, renderer.cluster_grid_resolution.x, renderer.cluster_grid_resolution.y, renderer.cluster_grid_resolution.z);
        }

        // NOTE: Light culling pass
        {
            vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, renderer.cluster_light_culling_pipeline);
            vkCmdDispatch(frame_resources->graphics_compute_command_buffer, 1, 1, renderer.cluster_grid_resolution.z / 4);
        }

    #if 0
        // NOTE: Voxelization render pass
        {
            render_pass_begin_info.renderPass = renderer.voxelization_render_pass;
            render_pass_begin_info.framebuffer = renderer.voxelization_pass_framebuffer;
            render_pass_begin_info.renderArea.offset = {0, 0};
            render_pass_begin_info.renderArea.extent = {renderer.voxel_grid_resolution.x, renderer.voxel_grid_resolution.x};
            vkCmdBeginRenderPass(frame_resources->graphics_compute_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.voxelizer_pipeline);

            VkViewport viewport = {0, 0, (f32)renderer.voxel_grid_resolution.x, (f32)renderer.voxel_grid_resolution.x, 0, 1};
            vkCmdSetViewport(frame_resources->graphics_compute_command_buffer, 0, 1, &viewport);

            VkRect2D scissor = {{0, 0}, {renderer.voxel_grid_resolution.x, renderer.voxel_grid_resolution.x}};
            vkCmdSetScissor(frame_resources->graphics_compute_command_buffer, 0, 1, &scissor);

            vkCmdDrawIndexedIndirectCountKHR(frame_resources->graphics_compute_command_buffer, renderer.frame_resources_buffer, frame_resources->draw_command_offset, renderer.frame_resources_buffer, frame_resources->draw_call_count_offset, MAX_DRAW_CALLS_PER_FRAME, sizeof(VkDrawIndexedIndirectCommand));

            vkCmdEndRenderPass(frame_resources->graphics_compute_command_buffer);
        }

        {
            sv3 voxel_grid_resolution = {(s32)renderer.voxel_grid_resolution.x, (s32)renderer.voxel_grid_resolution.y, (s32)renderer.voxel_grid_resolution.z};
            u32 voxel_texture_mipmap_count = most_significant_bit_index(max(max(voxel_grid_resolution.x, voxel_grid_resolution.y), voxel_grid_resolution.z)) + 1;

            VkImageMemoryBarrier voxel_image_barrier = {};
            voxel_image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            voxel_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            voxel_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            voxel_image_barrier.image = renderer.voxel_texture.image;
            voxel_image_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, voxel_texture_mipmap_count, 0, 1};
            voxel_image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            voxel_image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            voxel_image_barrier.srcAccessMask = 0;
            voxel_image_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            vkCmdPipelineBarrier(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &voxel_image_barrier);

            VkBufferMemoryBarrier voxel_buffer_barrier = {};
            voxel_buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            voxel_buffer_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            voxel_buffer_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            voxel_buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            voxel_buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            voxel_buffer_barrier.buffer = renderer.voxel_storage_buffer;
            voxel_buffer_barrier.size = renderer.voxel_storage_memory_block.size;
            vkCmdPipelineBarrier(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 1, &voxel_buffer_barrier, 0, NULL);

            // NOTE: Copy data from voxel buffer to voxel texture
            vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, renderer.voxelizer_compute_pipeline);
            vkCmdDispatch(frame_resources->graphics_compute_command_buffer, (u32)(renderer.voxel_grid_resolution.x * renderer.voxel_grid_resolution.y * renderer.voxel_grid_resolution.z) / 256, 1, 1);

            voxel_image_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            voxel_image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            voxel_image_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            voxel_image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &voxel_image_barrier);

            // NOTE: Generate mipmaps for voxel texture
            for (u32 i = 1; i < voxel_texture_mipmap_count; ++i)
            {
                voxel_image_barrier.subresourceRange.baseMipLevel = i - 1;
                voxel_image_barrier.subresourceRange.levelCount = 1;
                voxel_image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                voxel_image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                voxel_image_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                voxel_image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                vkCmdPipelineBarrier(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &voxel_image_barrier);

                VkImageBlit image_blit = {};
                image_blit.srcOffsets[0] = {0, 0, 0};
                image_blit.srcOffsets[1] = {voxel_grid_resolution.x, voxel_grid_resolution.y, voxel_grid_resolution.z};
                image_blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1};

                voxel_grid_resolution = {max(voxel_grid_resolution.x >> 1, 1), max(voxel_grid_resolution.y >> 1, 1), max(voxel_grid_resolution.z >> 1, 1)};

                image_blit.dstOffsets[0] = {0, 0, 0};
                image_blit.dstOffsets[1] = {voxel_grid_resolution.x, voxel_grid_resolution.y, voxel_grid_resolution.z};
                image_blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1};

                vkCmdBlitImage(frame_resources->graphics_compute_command_buffer, renderer.voxel_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, renderer.voxel_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit, VK_FILTER_LINEAR);
            }

            // NOTE: Transition the image layout of the last mipmap separately since after the blits it has a different layout than the other mip levels
            voxel_image_barrier.subresourceRange.baseMipLevel = voxel_texture_mipmap_count - 1;
            voxel_image_barrier.subresourceRange.levelCount = 1;
            voxel_image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            voxel_image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            voxel_image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            voxel_image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &voxel_image_barrier);

            // NOTE: Transition the image layouts of the rest of the mip levels
            voxel_image_barrier.subresourceRange.baseMipLevel = 0;
            voxel_image_barrier.subresourceRange.levelCount = voxel_texture_mipmap_count - 1;
            voxel_image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            voxel_image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            voxel_image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            voxel_image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &voxel_image_barrier);
        }
    #endif

        // NOTE: Gbuffer render pass
        {
            render_pass_begin_info.renderPass = renderer.main_render_pass;
            render_pass_begin_info.framebuffer = renderer.main_framebuffer;
            render_pass_begin_info.renderArea.offset = {0, 0};
            render_pass_begin_info.renderArea.extent = {renderer.render_dimensions.x, renderer.render_dimensions.y};
            render_pass_begin_info.clearValueCount = array_count(clear_values);
            render_pass_begin_info.pClearValues = clear_values;
            vkCmdBeginRenderPass(frame_resources->graphics_compute_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport = {(f32)render_pass_begin_info.renderArea.offset.x, (f32)render_pass_begin_info.renderArea.offset.y, (f32)render_pass_begin_info.renderArea.extent.width, (f32)render_pass_begin_info.renderArea.extent.height, 0, 1};
            vkCmdSetViewport(frame_resources->graphics_compute_command_buffer, 0, 1, &viewport);

            VkRect2D scissor = render_pass_begin_info.renderArea;
            vkCmdSetScissor(frame_resources->graphics_compute_command_buffer, 0, 1, &scissor);

            vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.main_pipeline);
            vkCmdDrawIndexedIndirectCountKHR(frame_resources->graphics_compute_command_buffer, renderer.frame_resources_buffer, frame_resources->draw_command_offset, renderer.frame_resources_buffer, frame_resources->draw_call_count_offset, MAX_DRAW_CALLS_PER_FRAME, sizeof(VkDrawIndexedIndirectCommand));

            if (renderer.draw_debug_cluster_grid)
            {
                vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.DEBUG_cluster_visualizer_pipeline);
                vkCmdDraw(frame_resources->graphics_compute_command_buffer, renderer.cluster_grid_resolution.x * renderer.cluster_grid_resolution.y * renderer.cluster_grid_resolution.z, 1, 0, 0);
            }

            // if (renderer.draw_debug_voxel_grid)
            // {
            //     vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.DEBUG_voxel_visualizer_pipeline);
            //     vkCmdDraw(frame_resources->graphics_compute_command_buffer, renderer.voxel_grid_resolution.x * renderer.voxel_grid_resolution.y * renderer.voxel_grid_resolution.z, 1, 0, 0);
            // }

            vkCmdEndRenderPass(frame_resources->graphics_compute_command_buffer);
        }

        // NOTE: Blit to swapchain render pass
        {
            render_pass_begin_info.renderPass = vulkan_context.swapchain_render_pass;
            render_pass_begin_info.framebuffer = vulkan_context.swapchain_framebuffers[frame_params->frame_number % MAX_FRAME_RESOURCES];

            // NOTE: Setting the render pass's render area like this means we can't control the aspect ratio 'bars' with the clear color (they will always be black)
            render_pass_begin_info.renderArea = aspect_ratio_corrected_render_area(renderer.render_dimensions.x, renderer.render_dimensions.y, vulkan_context.swapchain_extent.width, vulkan_context.swapchain_extent.height);

            vkCmdBeginRenderPass(frame_resources->graphics_compute_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(frame_resources->graphics_compute_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.final_pass_pipeline);

            VkViewport viewport = {(f32)render_pass_begin_info.renderArea.offset.x, (f32)render_pass_begin_info.renderArea.offset.y, (f32)render_pass_begin_info.renderArea.extent.width, (f32)render_pass_begin_info.renderArea.extent.height, 0, 1};
            vkCmdSetViewport(frame_resources->graphics_compute_command_buffer, 0, 1, &viewport);

            VkRect2D scissor = render_pass_begin_info.renderArea;
            vkCmdSetScissor(frame_resources->graphics_compute_command_buffer, 0, 1, &scissor);

            vkCmdDraw(frame_resources->graphics_compute_command_buffer, 3, 1, 0, 0);
            vkCmdEndRenderPass(frame_resources->graphics_compute_command_buffer);
        }

        VK_CALL(vkEndCommandBuffer(frame_resources->graphics_compute_command_buffer));

        frame_resources->should_record_command_buffer = false;
    }

    VkResult result = acquire_next_image(frame_resources->image_available_semaphore, &frame_resources->swapchain_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swapchain(vulkan_context.swapchain_extent.width, vulkan_context.swapchain_extent.height);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        // TODO: Logging
        DEBUG_printf("Unable to acquire Vulkan swapchain image!\n");
        return;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame_resources->image_available_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frame_resources->graphics_compute_command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &frame_resources->render_finished_semaphore;
    VK_CALL(vkQueueSubmit(vulkan_context.graphics_compute_queue, 1, &submit_info, frame_resources->graphics_compute_command_buffer_fence));

    VkPresentInfoKHR present_info = {};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame_resources->render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vulkan_context.swapchain;
    present_info.pImageIndices = &frame_resources->swapchain_image_index;

    result = vkQueuePresentKHR(vulkan_context.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreate_swapchain(vulkan_context.swapchain_extent.width, vulkan_context.swapchain_extent.height);
        return;
    }
    else if (result != VK_SUCCESS)
    {
        // TODO: Logging
        DEBUG_printf("Unable to present Vulkan swapchain image!\n");
        return;
    }
}

void renderer_resize(u32 window_width, u32 window_height)
{
    if (vulkan_context.device == NULL || vulkan_context.surface == NULL)
    {
        return;
    }
    recreate_swapchain(window_width, window_height);

    for (u32 i = 0; i < MAX_FRAME_RESOURCES; ++i)
    {
        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkDestroyFence(vulkan_context.device, renderer.frame_resources[i].graphics_compute_command_buffer_fence, NULL);
        VK_CALL(vkCreateFence(vulkan_context.device, &fence_create_info, NULL, &renderer.frame_resources[i].graphics_compute_command_buffer_fence));

        renderer.frame_resources[i].should_record_command_buffer = true;
    }
    vkResetCommandPool(vulkan_context.device, renderer.graphics_command_pool, 0);
}