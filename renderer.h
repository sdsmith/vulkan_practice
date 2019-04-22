#pragma once

#include "vk_error.h"
#include <algorithm>
#include <cassert>
#include <vulkan/vulkan.h>
#include <vector>

struct Swapchain_Buffer {
    VkImage image;
    VkImageView view;
};

struct Vulkan_Instance_Info
{
    VkApplicationInfo app_info;

    VkInstance instance;
    std::vector<char const*> instance_extension_names;
    std::vector<char const*> instance_layer_names;
    std::vector<char const*> device_extension_names;
    std::vector<char const*> device_layer_names;

    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    std::vector<Swapchain_Buffer> swapchain_buffers; //!< Swapchain image buffers

    struct Logical_Device
    {
        VkDevice device;
        VkCommandPool gr_cmd_pool;
        VkCommandBuffer gr_cmd_buf;
    } logical_device;
    
    struct System
    {
        std::vector<VkPhysicalDevice> physical_devices;
        struct Physical_Device
        {
            VkPhysicalDevice device;
            std::vector<VkQueueFamilyProperties> queue_family_properties;
            
            struct Queue
            {
                uint32_t gr_family_index;
                uint32_t present_family_index;
            } queue;
        } primary;
    } system;

    Status create_instance() {
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

    Status setup_primary_physical_device() {
        uint32_t dev_count = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, nullptr));
        if (dev_count == 0) {
            log_error("No devices available\n");
            return !STATUS_OK;
        }

        system.physical_devices.resize(dev_count);
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, system.physical_devices.data()));
        system.primary.device = system.physical_devices[0];

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

        return STATUS_OK;
    }

    Status find_graphics_and_present_queue() {
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

        for (size_t i = 0; i < queue_family_props.size(); ++i) {
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

    Status create_logical_device() {
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

        VK_CHECK(vkCreateDevice(system.primary.device, &device_info, nullptr, &logical_device.device));
        return STATUS_OK;
    }

    Status create_command_pool() {
        // NOTE: Need one pool for each type of queue being used.
        VkCommandPoolCreateInfo cmd_pool_ci = {};
        cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_ci.pNext = nullptr;
        cmd_pool_ci.queueFamilyIndex = system.primary.queue.gr_family_index;
        cmd_pool_ci.flags = 0;
        VK_CHECK(vkCreateCommandPool(logical_device.device, &cmd_pool_ci, nullptr, &logical_device.gr_cmd_pool));
        return STATUS_OK;
    }

    Status create_command_buffer() {
        VkCommandBufferAllocateInfo gr_cmd_buf_alloc_info = {};
        gr_cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        gr_cmd_buf_alloc_info.pNext = nullptr;
        gr_cmd_buf_alloc_info.commandPool = logical_device.gr_cmd_pool;
        gr_cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        gr_cmd_buf_alloc_info.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(logical_device.device, &gr_cmd_buf_alloc_info, &logical_device.gr_cmd_buf));
        return STATUS_OK;
    }

#ifdef _WIN32
    Status create_surface(Window const& window) {
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
    Status setup_swapchain(uint32_t num_buf_frames, uint32_t image_width, uint32_t image_height) {
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
        VkExtent2D swapchain_extent = {};
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

        VK_CHECK(vkCreateSwapchainKHR(logical_device.device, &swapchain_ci, nullptr, &swapchain));
        
        // DOC:
        //
        uint32_t swapchain_image_count = 0;
        VK_CHECK(vkGetSwapchainImagesKHR(logical_device.device, swapchain, &swapchain_image_count, nullptr));
        std::vector<VkImage> swapchain_images(swapchain_image_count);
        VK_CHECK(vkGetSwapchainImagesKHR(logical_device.device, swapchain, &swapchain_image_count, swapchain_images.data()));

        swapchain_buffers.resize(swapchain_image_count);
        for (uint32_t i = 0; i < swapchain_image_count; ++i) {
            swapchain_buffers[i].image = swapchain_images[i];
        }

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

            VK_CHECK(vkCreateImageView(logical_device.device, &color_image_view_ci, nullptr, &swapchain_buffers[i].view));
        }

        return STATUS_OK;
    }

    void cleanup() {
        for (Swapchain_Buffer& buf : swapchain_buffers) {
            vkDestroyImageView(logical_device.device, buf.view, nullptr);
        }
        vkDestroySwapchainKHR(logical_device.device, swapchain, nullptr);

        vkFreeCommandBuffers(logical_device.device, logical_device.gr_cmd_pool, 1 /*TODO: gr_cmd_buf_alloc_info.commandBufferCount*/, &logical_device.gr_cmd_buf);
        vkDestroyCommandPool(logical_device.device, logical_device.gr_cmd_pool, nullptr);
        vkDestroyDevice(logical_device.device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
};