#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct Vulkan
{
    VkApplicationInfo app_info;

    VkInstance instance;
    std::vector<char const*> extension_names;
    std::vector<char const*> layer_names;

    struct Logical_Device
    {
        VkDevice logical_device;
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

        struct Device
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
};