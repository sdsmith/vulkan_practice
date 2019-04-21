#pragma once

#include "vk_error.h"
#include <cassert>
#include <vulkan/vulkan.h>
#include <vector>

struct Vulkan_Instance_Info
{
    VkApplicationInfo app_info;

    VkInstance instance;
    std::vector<char const*> extension_names;
    std::vector<char const*> layer_names;

    struct Logical_Device
    {
        VkDevice device;
        VkCommandPool gr_cmd_pool;
        VkCommandBuffer gr_cmd_buf;
    } logical_device;
    
    struct Swapchain
    {
        VkSurfaceKHR surface;
        VkSurfaceCapabilitiesKHR surface_capabilities;
    } swapchain;
    
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
        inst_info.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
        inst_info.ppEnabledExtensionNames = extension_names.data();
        inst_info.enabledLayerCount = static_cast<uint32_t>(layer_names.size());
        inst_info.ppEnabledLayerNames = layer_names.data();

        return vkCreateInstance(&inst_info, nullptr, &instance);
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

#ifdef _WIN32
    Status create_surface(Window const& window) {
        VkWin32SurfaceCreateInfoKHR surface_create_info = {};
        surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_create_info.pNext = nullptr;
        surface_create_info.hinstance = window.h_instance;
        surface_create_info.hwnd = window.h_window;
        return vkCreateWin32SurfaceKHR(instance, &surface_create_info, nullptr, &swapchain.surface);
    }
#else
#   error Unsupported platform
#endif

    Status find_graphics_and_present_queue() {
        auto queue_family_props = system.primary.queue_family_properties;

        // Query queues that support presenting surfaces
        std::vector<VkBool32> queue_family_supports_present(queue_family_props.size(), false);
        for (uint32_t i = 0; i < queue_family_props.size(); ++i) {
            vkGetPhysicalDeviceSurfaceSupportKHR(system.primary.device, i, swapchain.surface, &queue_family_supports_present[i]);
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
        VkDeviceQueueCreateInfo gr_queue_create_info = {};
        gr_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        gr_queue_create_info.pNext = nullptr;
        gr_queue_create_info.queueCount = 1;
        gr_queue_create_info.pQueuePriorities = queue_priorities;
        gr_queue_create_info.queueFamilyIndex = system.primary.queue.gr_family_index;

        // Create logical device
        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = nullptr;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &gr_queue_create_info;
        device_info.enabledExtensionCount = 0;
        device_info.ppEnabledExtensionNames = nullptr;
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.pEnabledFeatures = nullptr;

        return vkCreateDevice(system.primary.device, &device_info, nullptr, &logical_device.device);
    }

    Status create_command_pool() {
        // NOTE: Need one pool for each type of queue being used.
        VkCommandPoolCreateInfo cmd_pool_create_info = {};
        cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_create_info.pNext = nullptr;
        cmd_pool_create_info.queueFamilyIndex = system.primary.queue.gr_family_index;
        cmd_pool_create_info.flags = 0;
        return vkCreateCommandPool(logical_device.device, &cmd_pool_create_info, nullptr, &logical_device.gr_cmd_pool);
    }

    Status create_command_buffer() {
        VkCommandBufferAllocateInfo gr_cmd_buf_alloc_info = {};
        gr_cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        gr_cmd_buf_alloc_info.pNext = nullptr;
        gr_cmd_buf_alloc_info.commandPool = logical_device.gr_cmd_pool;
        gr_cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        gr_cmd_buf_alloc_info.commandBufferCount = 1;
        VkCommandBuffer gr_cmd_buf = {};
        return vkAllocateCommandBuffers(logical_device.device, &gr_cmd_buf_alloc_info, &gr_cmd_buf);
    }

    /*
    Status setup_swapchain(uint32_t num_buf_frames, uint32_t window_width, uint32_t window_height) {
        for (size_t i = 0; i < system.primary.queue_family_properties.size(); ++i) {
            vkGetPhysicalDeviceSurfaceSupportKHR(system.primary.device, i, swapchain.surface, );
        }

        VkSwapchainCreateInfoKHR swapchain_create_info = {};
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.pNext = nullptr;
        swapchain_create_info.surface = swapchain.surface;
        swapchain_create_info.imageFormat = ; // TODO: pick the image format based on those available to the display (VkSurfaceFormatKHR)
        swapchain_create_info.minImageCount = num_buf_frames; // Set to the buffering strategy (double or triple buffer)
        swapchain_create_info.imageExtent.width = window_width;
        swapchain_create_info.imageExtent.height = window_height;
        swapchain_create_info.preTransform = ;
        swapchain_create_info.presentMode = ;
    }
    */

    void cleanup() {
        vkFreeCommandBuffers(logical_device.device, logical_device.gr_cmd_pool, 1 /*TODO: gr_cmd_buf_alloc_info.commandBufferCount*/, &logical_device.gr_cmd_buf);
        vkDestroyCommandPool(logical_device.device, logical_device.gr_cmd_pool, nullptr);
        vkDestroyDevice(logical_device.device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
};