#include "renderer.h"

#include "glm/ext/matrix_clip_space.hpp" // glm::perspective
#include "glm/ext/matrix_transform.hpp" // glm::lookAt
#include "vulkan_cube_data.h"
#include <algorithm>
#include <cassert>
#include <fstream>

Status Vulkan_Instance_Info::create_instance() {
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
    inst_info.ppEnabledExtensionNames = instance_extension_names.data();
    inst_info.enabledLayerCount = static_cast<uint32_t>(instance_layer_names.size());
    inst_info.ppEnabledLayerNames = instance_layer_names.data();

    VK_CHECK(vkCreateInstance(&inst_info, nullptr, &instance));
    return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_primary_physical_device() {
    uint32_t dev_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, nullptr));
    if (dev_count == 0) {
        log_error("No devices available\n");
        return !STATUS_OK;
    }

    system.physical_devices.resize(dev_count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, system.physical_devices.data()));
    
    // TODO: be lazy about it and pick the first available device.
    system.primary.device = system.physical_devices[0];

    VkPhysicalDeviceProperties dev_props = {};
    vkGetPhysicalDeviceProperties(system.primary.device, &dev_props);
    if (dev_props.apiVersion < app_info.apiVersion) {
        log_error("Vulkan version %u.%u not supported by device (max version %u.%u)\n",
            VK_VERSION_MAJOR(app_info.apiVersion), VK_VERSION_MINOR(app_info.apiVersion),
            VK_VERSION_MAJOR(dev_props.apiVersion), VK_VERSION_MINOR(dev_props.apiVersion));
        return !STATUS_OK;
    }

    // NOTE: A device defines types of queues that can perform specific work.
    // Each queue type is called a queue family. Each queue family may have one
    // or more queues available for use. A queue family may support one or more
    // type of work.
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(system.primary.device, &queue_family_count, nullptr);
    assert(queue_family_count > 0);
    system.primary.queue_family_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(system.primary.device, &queue_family_count,
        system.primary.queue_family_properties.data());

    vkGetPhysicalDeviceMemoryProperties(system.primary.device, &system.primary.memory_properties);

    return STATUS_OK;
}

Status Vulkan_Instance_Info::find_graphics_and_present_queue() {
    auto queue_family_props = system.primary.queue_family_properties;

    // Query queues that support presenting surfaces
    std::vector<VkBool32> queue_family_supports_present(queue_family_props.size(), false);
    for (uint32_t i = 0; i < queue_family_props.size(); ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(system.primary.device, i, surface, &queue_family_supports_present[i]);
    }

    // Find graphics and present queue, perferably one that supports both
    uint32_t& gr_fam_index = system.primary.queue.gr_family_index;
    uint32_t& present_fam_index = system.primary.queue.present_family_index;
    gr_fam_index = std::numeric_limits<uint32_t>::max();
    present_fam_index = std::numeric_limits<uint32_t>::max();

    for (uint32_t i = 0; i < queue_family_props.size(); ++i) {
        bool const supports_graphics = queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        if (supports_graphics) {
            gr_fam_index = i;
        }

        if (queue_family_supports_present[i]) {
            present_fam_index = i;
        }

        if (gr_fam_index == present_fam_index) {
            // found a queue that supports both grpahics and present, exit
            break;
        }
    }

    if (gr_fam_index == std::numeric_limits<uint32_t>::max()) {
        log_error("Unable to find a graphics queue on device\n");
        return !STATUS_OK;
    }
    if (present_fam_index == std::numeric_limits<uint32_t>::max()) {
        log_error("Unable to find a present queue on device\n");
        return !STATUS_OK;
    }

    return STATUS_OK;
}

Status Vulkan_Instance_Info::create_logical_device() {
    // TODO: setup the present queue (may be the same as the graphics queue)
    float queue_priorities[1] = { 0.0 };
    VkDeviceQueueCreateInfo gr_queue_ci = {};
    gr_queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    gr_queue_ci.pNext = nullptr;
    gr_queue_ci.queueCount = 1;
    gr_queue_ci.pQueuePriorities = queue_priorities;
    gr_queue_ci.queueFamilyIndex = system.primary.queue.gr_family_index;

    // Create logical device
    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = nullptr;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &gr_queue_ci;
    device_info.enabledExtensionCount = static_cast<uint32_t>(device_extension_names.size());
    device_info.ppEnabledExtensionNames = device_extension_names.data();
    device_info.enabledLayerCount = static_cast<uint32_t>(device_layer_names.size());
    device_info.ppEnabledLayerNames = device_layer_names.data();
    device_info.pEnabledFeatures = nullptr;

    VK_CHECK(vkCreateDevice(system.primary.device, &device_info, nullptr, &logical.device));
    return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_device_queue() {
    vkGetDeviceQueue(logical.device, system.primary.queue.gr_family_index, 0, &gr_queue);
    if (system.primary.queue.gr_family_index == system.primary.queue.present_family_index) {
        present_queue = gr_queue;
    }
    else {
        vkGetDeviceQueue(logical.device, system.primary.queue.present_family_index, 0, &present_queue);
    }

    return STATUS_OK;
}

Status Vulkan_Instance_Info::create_command_pool() {
    // NOTE: Need one pool for each type of queue being used.
    VkCommandPoolCreateInfo cmd_pool_ci = {};
    cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_ci.pNext = nullptr;
    cmd_pool_ci.queueFamilyIndex = system.primary.queue.gr_family_index;
    cmd_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(logical.device, &cmd_pool_ci, nullptr, &logical.gr_cmd_pool));
    return STATUS_OK;
}

Status Vulkan_Instance_Info::create_command_buffer() {
    VkCommandBufferAllocateInfo gr_cmd_buf_alloc_info = {};
    gr_cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    gr_cmd_buf_alloc_info.pNext = nullptr;
    gr_cmd_buf_alloc_info.commandPool = logical.gr_cmd_pool;
    gr_cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    gr_cmd_buf_alloc_info.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(logical.device, &gr_cmd_buf_alloc_info, &logical.gr_cmd_buf));
    return STATUS_OK;
}

#ifdef _WIN32
Status Vulkan_Instance_Info::create_surface(Window const& window) {
    VkWin32SurfaceCreateInfoKHR surface_ci = {};
    surface_ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_ci.pNext = nullptr;
    surface_ci.hinstance = window.h_instance;
    surface_ci.hwnd = window.h_window;
    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &surface_ci, nullptr, &surface));
    return STATUS_OK;
}
#else
#   error Unsupported platform
#endif

/**
 * \param num_buf_frames The number of frames in the buffering strategy.
 *  Clipped to the lowest supported number of frames.
 */
Status Vulkan_Instance_Info::setup_swapchain(uint32_t num_buf_frames, uint32_t image_width, uint32_t image_height) {
    // Get surface format support
    //
    uint32_t format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(system.primary.device, surface, &format_count, nullptr));
    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(system.primary.device, surface, &format_count, surface_formats.data()));

    // If format list contains a single entry of VK_FORMAT_UNDEFINED, the surface has no perferred format.
    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else {
        assert(format_count >= 1);
        swapchain_format = surface_formats[0].format;
    }

    // Get surface capabilities
    //
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(system.primary.device, surface, &surface_capabilities));

    uint32_t surface_present_mode_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(system.primary.device, surface,
        &surface_present_mode_count, nullptr));
    std::vector<VkPresentModeKHR> surface_present_modes(surface_present_mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(system.primary.device, surface,
        &surface_present_mode_count, surface_present_modes.data()));

    // Determine swapchain extent
    //
    // NOTE: Width and height are either both 0xffff_ffff, or neither has that value.
    static constexpr uint32_t undefined_extent = 0xffff'ffff;
    if (surface_capabilities.currentExtent.width == undefined_extent) {
        assert(surface_capabilities.currentExtent.height == undefined_extent);

        // If undefined, set to the requested image size
        swapchain_extent.width = image_width;
        swapchain_extent.height = image_height;

        // Clip to supported width
        if (swapchain_extent.width < surface_capabilities.minImageExtent.width) {
            swapchain_extent.width = surface_capabilities.minImageExtent.width;
        }
        else if (swapchain_extent.width > surface_capabilities.maxImageExtent.width) {
            swapchain_extent.width = surface_capabilities.maxImageExtent.width;
        }

        // Clip to supported height
        if (swapchain_extent.height < surface_capabilities.minImageExtent.height) {
            swapchain_extent.height = surface_capabilities.minImageExtent.height;
        }
        else if (swapchain_extent.height > surface_capabilities.maxImageExtent.height) {
            swapchain_extent.height = surface_capabilities.maxImageExtent.height;
        }
    }
    else {
        // If surface size is define, swap chain size must match
        swapchain_extent = surface_capabilities.currentExtent;
    }

    // FIFO present mode is guaranteed by the spec to be supported
    //
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // Determine the number of vkImages to use in the swap chain
    //
    uint32_t desired_num_swapchain_images = std::max(num_buf_frames, surface_capabilities.minImageCount);

    // DOC(sdryds):
    //
    VkSurfaceTransformFlagBitsKHR surface_pre_transform = {};
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surface_pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        surface_pre_transform = surface_capabilities.currentTransform;
    }

    // Find a supported composite alpha mode
    //
    // NOTE: One of the below values is guaranteed to be set.
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };

    // Take first supported
    for (uint32_t i = 0; i < sizeof(composite_alpha_flags); ++i) {
        if (surface_capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
            composite_alpha = composite_alpha_flags[i];
            break;
        }
    }

    // Create the swapchain
    //
    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = nullptr;
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = desired_num_swapchain_images;
    swapchain_ci.imageFormat = swapchain_format;
    swapchain_ci.imageExtent.width = swapchain_extent.width;
    swapchain_ci.imageExtent.height = swapchain_extent.height;
    swapchain_ci.preTransform = surface_pre_transform;
    swapchain_ci.compositeAlpha = composite_alpha;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.presentMode = swapchain_present_mode;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
    swapchain_ci.clipped = VK_TRUE;
    swapchain_ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = nullptr;

    // Account for seperate graphics and present queues
    uint32_t queue_family_indicies[2] = {
        system.primary.queue.gr_family_index,
        system.primary.queue.present_family_index
    };

    if (system.primary.queue.gr_family_index != system.primary.queue.present_family_index) {
        // If the graphics and present queues are from different queue families we have two options:
        // 1) explicitly transfer the ownership of images between the queues
        // 2) create the swapchain with imageSharingMode as VK_SHARING_MODE_CONCURRENT
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = 2;
        swapchain_ci.pQueueFamilyIndices = queue_family_indicies;
    }

    VK_CHECK(vkCreateSwapchainKHR(logical.device, &swapchain_ci, nullptr, &swapchain));

    // Get the swapchain image handles
    //
    uint32_t swapchain_image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(logical.device, swapchain, &swapchain_image_count, nullptr));
    std::vector<VkImage> swapchain_images(swapchain_image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(logical.device, swapchain, &swapchain_image_count, swapchain_images.data()));

    swapchain_buffers.resize(swapchain_image_count);
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        swapchain_buffers[i].image = swapchain_images[i];
    }

    // Create image views for each swapchain image
    //
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        VkImageViewCreateInfo color_image_view_ci = {};
        color_image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view_ci.pNext = nullptr;
        color_image_view_ci.flags = 0;
        color_image_view_ci.image = swapchain_buffers[i].image;
        color_image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view_ci.format = swapchain_format;
        color_image_view_ci.components.r = VK_COMPONENT_SWIZZLE_R;
        color_image_view_ci.components.g = VK_COMPONENT_SWIZZLE_G;
        color_image_view_ci.components.b = VK_COMPONENT_SWIZZLE_B;
        color_image_view_ci.components.a = VK_COMPONENT_SWIZZLE_A;
        color_image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view_ci.subresourceRange.baseMipLevel = 0;
        color_image_view_ci.subresourceRange.levelCount = 1;
        color_image_view_ci.subresourceRange.baseArrayLayer = 0;
        color_image_view_ci.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(logical.device, &color_image_view_ci, nullptr, &swapchain_buffers[i].view));
    }

    return STATUS_OK;
}

bool Vulkan_Instance_Info::memory_type_from_properties(uint32_t type_bits, VkFlags requirements_mask, uint32_t * type_index) {
    // Seach memory types and find the first index with desired properties
    for (uint32_t i = 0; i < system.primary.memory_properties.memoryTypeCount; ++i) {
        if ((type_bits & 1) == 1) {
            // Type is available

            if ((system.primary.memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                *type_index = i;
                return true;
            }
        }

        type_bits >>= 1;
    }

    // No matches
    return false;
}


Status Vulkan_Instance_Info::setup_depth_buffer() {
    /*
     * NOTE: Not required to initialize memory. It is handled by the device.
     */

    static constexpr VkSampleCountFlagBits sample_flag_bits = VK_SAMPLE_COUNT_1_BIT;

    VkImageCreateInfo image_ci = {};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.pNext = nullptr;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_D16_UNORM;
    image_ci.extent.width = swapchain_extent.width;
    image_ci.extent.height = swapchain_extent.height;
    image_ci.extent.depth = 1;
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = sample_flag_bits;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_ci.queueFamilyIndexCount = 0;
    image_ci.pQueueFamilyIndices = nullptr;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.flags = 0;

    VK_CHECK(vkCreateImage(logical.device, &image_ci, nullptr, &depth_buf.image));
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(logical.device, depth_buf.image, &mem_reqs);

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = nullptr;
    mem_alloc.allocationSize = mem_reqs.size;
    if (!memory_type_from_properties(mem_reqs.memoryTypeBits, 0, &mem_alloc.memoryTypeIndex)) {
        log_error("Unable to find suitable memory for depth buffer.\n");
        return !STATUS_OK;
    }

    VK_CHECK(vkAllocateMemory(logical.device, &mem_alloc, nullptr, &depth_buf.mem));

    VK_CHECK(vkBindImageMemory(logical.device, depth_buf.image, depth_buf.mem, 0));

    VkImageViewCreateInfo view_ci = {};
    view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.pNext = nullptr;
    view_ci.image = depth_buf.image;
    view_ci.format = VK_FORMAT_D16_UNORM;
    view_ci.components.r = VK_COMPONENT_SWIZZLE_R;
    view_ci.components.g = VK_COMPONENT_SWIZZLE_G;
    view_ci.components.b = VK_COMPONENT_SWIZZLE_B;
    view_ci.components.a = VK_COMPONENT_SWIZZLE_A;
    view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_ci.subresourceRange.baseMipLevel = 0;
    view_ci.subresourceRange.levelCount = 1;
    view_ci.subresourceRange.baseArrayLayer = 0;
    view_ci.subresourceRange.layerCount = 1;
    view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_ci.flags = 0;

    VK_CHECK(vkCreateImageView(logical.device, &view_ci, nullptr, &depth_buf.view));

    return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_model_view_projection() {
    projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    view = glm::lookAt(glm::vec3(-5, 3, -10), // camera pos in world space
        glm::vec3(0, 0, 0),    // look at origin
        glm::vec3(0, -1, 0));  // head is up
    model = glm::mat4(1.0f);

    // Vulkan clip space has inverted Y and half Z.
    clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f);

    mvp = clip * projection * view * model;

    return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_uniform_buffer() {
    VkBufferCreateInfo buf_ci = {};
    buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.pNext = nullptr;
    buf_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_ci.size = sizeof(mvp);
    buf_ci.queueFamilyIndexCount = 0;
    buf_ci.pQueueFamilyIndices = nullptr;
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_ci.flags = 0;
    VK_CHECK(vkCreateBuffer(logical.device, &buf_ci, nullptr, &uniform_data.buf));

    VkMemoryRequirements mem_reqs = {};
    vkGetBufferMemoryRequirements(logical.device, uniform_data.buf, &mem_reqs);
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = nullptr;
    mem_alloc.memoryTypeIndex = 0;
    mem_alloc.allocationSize = mem_reqs.size;
    if (!memory_type_from_properties(mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &mem_alloc.memoryTypeIndex))
    {
        log_error("Unable to find suitable memory for uniform buffer\n");
    }

    VK_CHECK(vkAllocateMemory(logical.device, &mem_alloc, nullptr, &uniform_data.mem));

    // Initialize the uniform buffer
    //
    void* data = nullptr;
    VK_CHECK(vkMapMemory(logical.device, uniform_data.mem, 0, mem_reqs.size, 0, (void**)&data));
    memcpy(data, &mvp, sizeof(mvp));
    vkUnmapMemory(logical.device, uniform_data.mem);

    VK_CHECK(vkBindBufferMemory(logical.device, uniform_data.buf, uniform_data.mem, 0));

    // Record the uniform buffer information
    //
    uniform_data.buf_info.buffer = uniform_data.buf;
    uniform_data.buf_info.offset = 0;
    uniform_data.buf_info.range = sizeof(mvp);

    return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_pipeline() {
    // Descriptor set layouts
    //
    // Layout binding
    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.binding = 0;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_binding.pImmutableSamplers = nullptr;

    // Descriptor set layout
    constexpr uint32_t num_descriptor_sets = 1;
    VkDescriptorSetLayoutCreateInfo desc_layout_ci = {};
    desc_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc_layout_ci.pNext = nullptr;
    desc_layout_ci.bindingCount = 1;
    desc_layout_ci.pBindings = &layout_binding;
    desc_set_layouts.resize(num_descriptor_sets);
    VK_CHECK(vkCreateDescriptorSetLayout(logical.device, &desc_layout_ci, nullptr, desc_set_layouts.data()));

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = nullptr;
    pipeline_layout_ci.pushConstantRangeCount = 0;
    pipeline_layout_ci.pPushConstantRanges = nullptr;
    pipeline_layout_ci.setLayoutCount = num_descriptor_sets;
    pipeline_layout_ci.pSetLayouts = desc_set_layouts.data();

    VK_CHECK(vkCreatePipelineLayout(logical.device, &pipeline_layout_ci, nullptr, &pipeline_layout));

    // Create descriptor pool
    //
    VkDescriptorPoolSize type_count[1];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo desc_pool_ci = {};
    desc_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_ci.pNext = nullptr;
    desc_pool_ci.maxSets = 1;
    desc_pool_ci.poolSizeCount = 1;
    desc_pool_ci.pPoolSizes = type_count;
    VK_CHECK(vkCreateDescriptorPool(logical.device, &desc_pool_ci, nullptr, &desc_pool));

    // Allocate descriptor sets
    //
    VkDescriptorSetAllocateInfo alloc_info[1];
    alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info[0].pNext = nullptr;
    alloc_info[0].descriptorPool = desc_pool;
    alloc_info[0].descriptorSetCount = num_descriptor_sets;
    alloc_info[0].pSetLayouts = desc_set_layouts.data();
    desc_sets.resize(num_descriptor_sets);
    VK_CHECK(vkAllocateDescriptorSets(logical.device, alloc_info, desc_sets.data()));

    // Write the descriptor buffer info the device descriptor memory
    //
    // NOTE: It is likely in the devices memory, but not guaranteed to be.
    //
    VkWriteDescriptorSet writes[1];
    writes[0] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = nullptr;
    writes[0].dstSet = desc_sets[0];
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &uniform_data.buf_info;
    writes[0].dstArrayElement = 0;
    writes[0].dstBinding = 0;
    vkUpdateDescriptorSets(logical.device, 1, writes, 0, nullptr);

    return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_render_pass() {
    /*
     * Render pass consists of a collection of attachements, subpasses, and dependencies.
     */

	// TODO: semaphore missing???????

    // Attachments
    //
    // One for color and one for depth.
    constexpr int num_attachments = 2;
    constexpr int color_attachment_index = 0;
    constexpr int depth_attachment_index = 1;

    VkAttachmentDescription attachment_descs[num_attachments];

    attachment_descs[color_attachment_index].format = swapchain_format;
    attachment_descs[color_attachment_index].samples = num_samples;
    attachment_descs[color_attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Clear existing buf content
    attachment_descs[color_attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Keep the buf populated so we can display content later
    attachment_descs[color_attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[color_attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[color_attachment_index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Don't care what the start format is
    attachment_descs[color_attachment_index].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Final format should be optimal for presenting
    attachment_descs[color_attachment_index].flags = 0;

	attachment_descs[depth_attachment_index].format = VK_FORMAT_D16_UNORM;// TODO: should it be swapchain_format?;
    attachment_descs[depth_attachment_index].samples = num_samples;
    attachment_descs[depth_attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[depth_attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[depth_attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[depth_attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[depth_attachment_index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Don't care what the start format is
    attachment_descs[depth_attachment_index].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;  // Final format should be optimal for depth buffer
    attachment_descs[depth_attachment_index].flags = 0;

    // Subpasses
    //
    VkAttachmentReference color_ref = {};
    color_ref.attachment = color_attachment_index; // Index into the att
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref = {};
    depth_ref.attachment = depth_attachment_index;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Is graphics subpass
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depth_ref;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    // Render pass
    //
    VkRenderPassCreateInfo render_pass_ci = {};
    render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_ci.pNext = nullptr;
    render_pass_ci.attachmentCount = num_attachments;
    render_pass_ci.pAttachments = attachment_descs;
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpass;
    render_pass_ci.dependencyCount = 0;
    render_pass_ci.pDependencies = nullptr;
    VK_CHECK(vkCreateRenderPass(logical.device, &render_pass_ci, nullptr, &render_pass));

    return STATUS_OK;
}

namespace {
	Status load_spirv(char const* path, std::vector<uint32_t>* buf) {
		assert(path);
		assert(buf);

		std::ifstream f(path, std::ios_base::in | std::ios_base::binary);
		assert(f.is_open() && !f.fail());
		
		f.seekg(0, f.end);
		const std::streampos byte_length = f.tellg();
		f.seekg(0, f.beg);
		
		buf->clear();
		buf->resize(static_cast<size_t>(std::ceilf(byte_length / static_cast<float>(sizeof(uint32_t)))), 0U);
		f.read(reinterpret_cast<char*>(buf->data()), byte_length);

		f.close();

		return STATUS_OK;
	}
}

Status Vulkan_Instance_Info::setup_shaders() {
	Status status;
	VkShaderModuleCreateInfo module_ci = {};

	std::vector<uint32_t> shader_vert_spv;
	status = load_spirv("simple.vert.spv", &shader_vert_spv);
	if (status != STATUS_OK) { return status; }

	shader_stages_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages_ci[0].pNext = nullptr;
	shader_stages_ci[0].pSpecializationInfo = nullptr;
	shader_stages_ci[0].flags = 0;
	shader_stages_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages_ci[0].pName = "main";

	module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_ci.pNext = nullptr;
	module_ci.flags = 0;
	module_ci.codeSize = shader_vert_spv.size() * sizeof(decltype(shader_vert_spv)::value_type);
	module_ci.pCode = shader_vert_spv.data();
	VK_CHECK(vkCreateShaderModule(logical.device, &module_ci, nullptr, &shader_stages_ci[0].module));

	std::vector<uint32_t> shader_frag_spv;
	status = load_spirv("simple.frag.spv", &shader_frag_spv);
	if (status != STATUS_OK) { return status; }

	shader_stages_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages_ci[1].pNext = nullptr;
	shader_stages_ci[1].pSpecializationInfo = nullptr;
	shader_stages_ci[1].flags = 0;
	shader_stages_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages_ci[1].pName = "main";

	module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_ci.pNext = nullptr;
	module_ci.flags = 0;
	module_ci.codeSize = shader_frag_spv.size() * sizeof(decltype(shader_frag_spv)::value_type);
	module_ci.pCode = shader_frag_spv.data();
	VK_CHECK(vkCreateShaderModule(logical.device, &module_ci, nullptr, &shader_stages_ci[1].module));

	return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_framebuffer() {
	/*
	 * Attachement 0 is the swapchain buffer.
	 * Attachement 1 is the depth buffer (shared amung the framebuffers).
     */

	static constexpr uint32_t num_attachments = 2;
	VkImageView attachment_views[num_attachments];
	attachment_views[1] = depth_buf.view;

	VkFramebufferCreateInfo fb_ci = {};
	fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_ci.pNext = nullptr;
	fb_ci.renderPass = render_pass;
	fb_ci.attachmentCount = num_attachments;
	fb_ci.pAttachments = attachment_views;
	fb_ci.width = swapchain_extent.width; // TODO: Might need to make this the actual window dim, not the extent
	fb_ci.height = swapchain_extent.height;
	fb_ci.layers = 1;

	const size_t num_swapchain_images = swapchain_buffers.size();
	framebuffers.resize(num_swapchain_images);
	for (uint32_t i = 0; i < num_swapchain_images; ++i) {
		attachment_views[0] = swapchain_buffers[i].view;
		VK_CHECK(vkCreateFramebuffer(logical.device, &fb_ci, nullptr, &framebuffers[i]));
	}

	return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_vertex_buffer() {
	VkBufferCreateInfo vert_buf_ci = {};
	vert_buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vert_buf_ci.pNext = nullptr;
	vert_buf_ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vert_buf_ci.size = sizeof(Cube_Model::vertex_buffer_solid_face_colors_data);
	vert_buf_ci.queueFamilyIndexCount = 0;
	vert_buf_ci.pQueueFamilyIndices = nullptr;
	vert_buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vert_buf_ci.flags = 0;
	VK_CHECK(vkCreateBuffer(logical.device, &vert_buf_ci, nullptr, &vertex_buffer.buf));

	VkMemoryRequirements mem_reqs = {};
	vkGetBufferMemoryRequirements(logical.device, vertex_buffer.buf, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.memoryTypeIndex = 0;
	alloc_info.allocationSize = mem_reqs.size;
	if (!memory_type_from_properties(mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&alloc_info.memoryTypeIndex))
	{
		log_error("No mappable coherent memory");
		return !STATUS_OK;
	}

	VK_CHECK(vkAllocateMemory(logical.device, &alloc_info, nullptr, &vertex_buffer.mem));

	uint8_t* pData = nullptr;
	VK_CHECK(vkMapMemory(logical.device, vertex_buffer.mem, 0, mem_reqs.size, 0, reinterpret_cast<void**>(&pData)));

	memcpy(pData, Cube_Model::vertex_buffer_solid_face_colors_data, 
		sizeof(Cube_Model::vertex_buffer_solid_face_colors_data));

	vkUnmapMemory(logical.device, vertex_buffer.mem);

	VK_CHECK(vkBindBufferMemory(logical.device, vertex_buffer.buf, vertex_buffer.mem, 0));

	vertex_input_binding.binding = 0;
	vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertex_input_binding.stride = sizeof(Cube_Model::vertex_buffer_solid_face_colors_data[0]);

	vertex_input_attribs[0].binding = 0;
	vertex_input_attribs[0].location = 0;
	vertex_input_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertex_input_attribs[0].offset = 0;

	vertex_input_attribs[1].binding = 0;
	vertex_input_attribs[1].location = 1;
	vertex_input_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertex_input_attribs[1].offset = 16;

	return STATUS_OK;
}

Status Vulkan_Instance_Info::setup_graphics_pipeline() {
	// Dynamic state
	//
    // State that can be cahnged by a command buffer during command buffer execution.
	VkDynamicState dynamic_state_enables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo dynamic_state_ci = {};
	memset(dynamic_state_enables, 0, sizeof(dynamic_state_enables));
	dynamic_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_ci.pNext = nullptr;
	dynamic_state_ci.dynamicStateCount = 0;
	dynamic_state_ci.pDynamicStates = dynamic_state_enables;

    // Pipeline vertex input state
    //
    // Format and arrangement of vertex data
    VkPipelineVertexInputStateCreateInfo vert_input_state_ci = {};
    vert_input_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_input_state_ci.pNext = nullptr;
    vert_input_state_ci.flags = 0;
    vert_input_state_ci.vertexBindingDescriptionCount = 1;
    vert_input_state_ci.pVertexBindingDescriptions = &vertex_input_binding;
    vert_input_state_ci.vertexAttributeDescriptionCount = 2;
    vert_input_state_ci.pVertexAttributeDescriptions = vertex_input_attribs;

	// Pipeline vertex input assembly state
    //
    // Description of how vertex data forms the desired geometry
    VkPipelineInputAssemblyStateCreateInfo vert_input_asm_state_ci = {};
    vert_input_asm_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vert_input_asm_state_ci.pNext = nullptr;
    vert_input_asm_state_ci.flags = 0;
    vert_input_asm_state_ci.primitiveRestartEnable = VK_FALSE;
    vert_input_asm_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    // Pipeline rasterization state
    //
    VkPipelineRasterizationStateCreateInfo rasterization_state_ci = {};
    rasterization_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_ci.pNext = nullptr;
    rasterization_state_ci.flags = 0;
    rasterization_state_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_ci.depthClampEnable = VK_FALSE;
    rasterization_state_ci.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_ci.depthBiasEnable = VK_FALSE;
    rasterization_state_ci.depthBiasConstantFactor = 0;
    rasterization_state_ci.depthBiasClamp = 0;
    rasterization_state_ci.depthBiasSlopeFactor = 0;
    rasterization_state_ci.lineWidth = 1.0f;

    // Pipeline color blend state
    //
    // Configures replacement of pixels in the destination.
    //
    // One attachement state per attachement in the pipeline.
    VkPipelineColorBlendAttachmentState color_blend_attachment_state[1];
    color_blend_attachment_state[0].colorWriteMask = 0xf;
    color_blend_attachment_state[0].blendEnable = VK_FALSE;
    color_blend_attachment_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state[0].colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    
    VkPipelineColorBlendStateCreateInfo color_blend_state_ci = {};
    color_blend_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_ci.pNext = nullptr;
    color_blend_state_ci.flags = 0;
    color_blend_state_ci.attachmentCount = 1;
    color_blend_state_ci.pAttachments = color_blend_attachment_state;
    color_blend_state_ci.logicOpEnable = VK_FALSE;
    color_blend_state_ci.logicOp = VK_LOGIC_OP_NO_OP;
    color_blend_state_ci.blendConstants[0] = 1.0f;
    color_blend_state_ci.blendConstants[1] = 1.0f;
    color_blend_state_ci.blendConstants[2] = 1.0f;
    color_blend_state_ci.blendConstants[3] = 1.0f;

    // Pipeline viewport state
    //
    VkPipelineViewportStateCreateInfo viewport_state_ci = {};
    viewport_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_ci.pNext = nullptr;
    viewport_state_ci.flags = 0;
    viewport_state_ci.viewportCount = num_viewports;
    dynamic_state_enables[dynamic_state_ci.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    viewport_state_ci.pViewports = nullptr;
    viewport_state_ci.scissorCount = num_scissors;
    dynamic_state_enables[dynamic_state_ci.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    viewport_state_ci.pScissors = nullptr;

    // Pipeline depth stencil state
    //
    // Setup fixed-function state: depth buffer and stencil.
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_ci = {};
    depth_stencil_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_ci.pNext = nullptr;
    depth_stencil_state_ci.flags = 0;
    depth_stencil_state_ci.depthTestEnable = VK_TRUE;
    depth_stencil_state_ci.depthWriteEnable = VK_TRUE;
    depth_stencil_state_ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state_ci.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_ci.minDepthBounds = 0;
    depth_stencil_state_ci.maxDepthBounds = 0;
    depth_stencil_state_ci.stencilTestEnable = VK_FALSE;
    depth_stencil_state_ci.back.failOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state_ci.back.passOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state_ci.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depth_stencil_state_ci.back.compareMask = 0;
    depth_stencil_state_ci.back.reference = 0;
    depth_stencil_state_ci.back.depthFailOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state_ci.back.writeMask = 0;
    depth_stencil_state_ci.front = depth_stencil_state_ci.back;

    // Pipeline multisample state
    //
    VkPipelineMultisampleStateCreateInfo multisample_state_ci = {};
    multisample_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_ci.pNext = nullptr;
    multisample_state_ci.flags = 0;
    multisample_state_ci.pSampleMask = nullptr;
    multisample_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_ci.sampleShadingEnable = VK_FALSE;
    multisample_state_ci.alphaToCoverageEnable = VK_FALSE;
    multisample_state_ci.alphaToOneEnable = VK_FALSE;
    multisample_state_ci.minSampleShading = 0.0f;

    // Graphics pipeline
    //
    VkGraphicsPipelineCreateInfo pipeline_ci = {};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = nullptr;
    pipeline_ci.layout = pipeline_layout;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = 0;
    pipeline_ci.flags = 0;
    pipeline_ci.pVertexInputState = &vert_input_state_ci;
    pipeline_ci.pInputAssemblyState = &vert_input_asm_state_ci;
    pipeline_ci.pRasterizationState = &rasterization_state_ci;
    pipeline_ci.pColorBlendState = &color_blend_state_ci;
    pipeline_ci.pTessellationState = nullptr;
    pipeline_ci.pMultisampleState = &multisample_state_ci;
    pipeline_ci.pDynamicState = &dynamic_state_ci;
    pipeline_ci.pViewportState = &viewport_state_ci;
    pipeline_ci.pDepthStencilState = &depth_stencil_state_ci;
    pipeline_ci.pStages = shader_stages_ci;
    pipeline_ci.stageCount = 2;
    pipeline_ci.renderPass = render_pass;
    pipeline_ci.subpass = 0;
    VK_CHECK(vkCreateGraphicsPipelines(logical.device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &pipeline));

    return STATUS_OK;
}

Status Vulkan_Instance_Info::render() {
    // Get swapchain image to render into

    const VkDeviceSize offsets[1] = { 0 };

    static constexpr uint32_t num_clear_values = 2;
    VkClearValue clear_values[num_clear_values];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth = 1.0f; // farthest away
    clear_values[1].depthStencil.stencil = 0;

    VkSemaphoreCreateInfo image_acquired_sema_ci = {};
    image_acquired_sema_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    image_acquired_sema_ci.pNext = nullptr;
    image_acquired_sema_ci.flags = 0;
    VK_CHECK(vkCreateSemaphore(logical.device, &image_acquired_sema_ci, nullptr,
        &image_acquired_sema));

    // Get next available swapchain image
    VK_CHECK(vkAcquireNextImageKHR(logical.device, swapchain, UINT64_MAX, image_acquired_sema,
        VK_NULL_HANDLE, &current_image));

    VK_CHECK(exec_begin_gr_command_buffer());
    {
        // Begin render pass
        VkRenderPassBeginInfo render_pass_begin;
        render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin.pNext = nullptr;
        render_pass_begin.renderPass = render_pass;
        assert(current_image < framebuffers.size());
        render_pass_begin.framebuffer = framebuffers[current_image];
        render_pass_begin.renderArea.offset.x = 0;
        render_pass_begin.renderArea.offset.y = 0;
        render_pass_begin.renderArea.extent.width = swapchain_extent.width;
        render_pass_begin.renderArea.extent.height = swapchain_extent.height;
        render_pass_begin.clearValueCount = num_clear_values;
        render_pass_begin.pClearValues = clear_values;
        vkCmdBeginRenderPass(logical.gr_cmd_buf, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

        // Bind vertex buf
        vkCmdBindVertexBuffers(
            logical.gr_cmd_buf,
            0, // Start binding
            1, // Binding count
            &vertex_buffer.buf, // pBuffers
            offsets); // pOffsets

        // Bind pipeline
        //
        // Describes how to render primatives.
        vkCmdBindPipeline(logical.gr_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        // Bind descriptor sets
        //
        // Describes shader input
        vkCmdBindDescriptorSets(logical.gr_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, desc_sets.data(), 0, nullptr);

        // Bind vertex buffer
        //
        const VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(logical.gr_cmd_buf, 0, 1, &vertex_buffer.buf, offsets);

        // Set viewport and scissor rectangle
        //
        // NOTE: Able to set in command buffer due to viewport and scissor state being dynamic.
        viewport.height = static_cast<float>(swapchain_extent.height);
        viewport.width = static_cast<float>(swapchain_extent.width);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(logical.gr_cmd_buf, 0, num_viewports, &viewport);

        scissor.extent.width = swapchain_extent.width;
        scissor.extent.height = swapchain_extent.height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(logical.gr_cmd_buf, 0, num_scissors, &scissor);

        // Draw
        //
        vkCmdDraw(logical.gr_cmd_buf, Cube_Model::vertex_count, 1, 0, 0);
        vkCmdEndRenderPass(logical.gr_cmd_buf);
    }
    VK_CHECK(exec_end_gr_command_buffer());

    // Transition swapchain image for present
    //
    // NOTE: Handled by setting the attachment description. Could also use a memory barrier approach.

    // Submit the command buffer
    //
    
    // Fense to wait for GPU completion
    VkFenceCreateInfo fence_ci = {};
    VkFence draw_fence;
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.pNext = nullptr;
    fence_ci.flags = 0;
    vkCreateFence(logical.device, &fence_ci, nullptr, &draw_fence);

    // Wait at the color attachment stage until swapchain image is available before writing colors.
    const std::vector<VkCommandBuffer> cmd_bufs = { logical.gr_cmd_buf };
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage when final color values are output from pipeline
    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = nullptr;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    submit_info[0].pWaitSemaphores = &image_acquired_sema;
    submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
    submit_info[0].commandBufferCount = static_cast<uint32_t>(cmd_bufs.size());
    submit_info[0].pCommandBuffers = cmd_bufs.data();
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = nullptr;
    VK_CHECK(vkQueueSubmit(gr_queue, 1, submit_info, draw_fence));

    // Wait for GPU completion
    static constexpr uint64_t fence_timeout = 100000000;

    VkResult res;
    do {
        res = vkWaitForFences(logical.device, 1, &draw_fence, VK_TRUE, fence_timeout);
    } while (res == VK_TIMEOUT);
    VK_CHECK(res);

    // Present
    //
    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &current_image;
    present_info.pWaitSemaphores = nullptr;
    present_info.waitSemaphoreCount = 0;
    present_info.pResults = nullptr;
    VK_CHECK(vkQueuePresentKHR(present_queue, &present_info));

    // TODO: move to single instantiation
    vkDestroyFence(logical.device, draw_fence, nullptr);

    return STATUS_OK;
}

void Vulkan_Instance_Info::cleanup() {
    vkDestroyPipeline(logical.device, pipeline, nullptr);

	vkDestroySemaphore(logical.device, image_acquired_sema, nullptr);
	vkFreeMemory(logical.device, vertex_buffer.mem, nullptr);
	vkDestroyBuffer(logical.device, vertex_buffer.buf, nullptr);


	for (uint32_t i = 0; i < framebuffers.size(); ++i) {
		vkDestroyFramebuffer(logical.device, framebuffers[i], nullptr);
	}

	vkDestroyShaderModule(logical.device, shader_stages_ci[0].module, nullptr);
	vkDestroyShaderModule(logical.device, shader_stages_ci[1].module, nullptr);

    vkDestroyRenderPass(logical.device, render_pass, nullptr);
    vkDestroyDescriptorPool(logical.device, desc_pool, nullptr);

    vkDestroyPipelineLayout(logical.device, pipeline_layout, nullptr);
    for (VkDescriptorSetLayout& desc_set_layout : desc_set_layouts) {
        vkDestroyDescriptorSetLayout(logical.device, desc_set_layout, nullptr);
    }

    vkFreeMemory(logical.device, uniform_data.mem, nullptr);
    vkDestroyBuffer(logical.device, uniform_data.buf, nullptr);

    vkFreeMemory(logical.device, depth_buf.mem, nullptr);
    vkDestroyImageView(logical.device, depth_buf.view, nullptr);
    vkDestroyImage(logical.device, depth_buf.image, nullptr);

    for (Swapchain_Buffer& buf : swapchain_buffers) {
        vkDestroyImageView(logical.device, buf.view, nullptr);
    }
    vkDestroySwapchainKHR(logical.device, swapchain, nullptr);

    vkFreeCommandBuffers(logical.device, logical.gr_cmd_pool, 1 /*TODO: gr_cmd_buf_alloc_info.commandBufferCount*/, &logical.gr_cmd_buf);
    vkDestroyCommandPool(logical.device, logical.gr_cmd_pool, nullptr);
    vkDestroyDevice(logical.device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

VkResult Vulkan_Instance_Info::exec_begin_gr_command_buffer() {
	VkCommandBufferBeginInfo cmd_buf_begin_info = {};
	cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_begin_info.pNext = nullptr;
	cmd_buf_begin_info.flags = 0;
	cmd_buf_begin_info.pInheritanceInfo = nullptr;

	return vkBeginCommandBuffer(logical.gr_cmd_buf, &cmd_buf_begin_info);
}

VkResult Vulkan_Instance_Info::exec_end_gr_command_buffer() {
	return vkEndCommandBuffer(logical.gr_cmd_buf);
}
