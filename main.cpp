#include "vk_error.h"

#include <cassert>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

int main()
{
    constexpr char const* app_name = "Vulkan Practice";
    constexpr uint32_t app_ver = 1;
    constexpr char const* engine_name = "Vulkan Practice Engine";
    constexpr uint32_t engine_ver = 1;

    std::vector<const char*> layers;
#if defined(_DEBUG)
    layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = app_ver;
    app_info.pEngineName = engine_name;
    app_info.engineVersion = engine_ver;
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount = 0;
    inst_info.ppEnabledExtensionNames = nullptr;
    inst_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    inst_info.ppEnabledLayerNames = layers.data();

    // Create Vulkan instance
    //
    VkInstance instance;
    VK_CHECK(vkCreateInstance(&inst_info, nullptr, &instance));

    // Setup physical device
    //
    uint32_t dev_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, nullptr));
    if (dev_count == 0) {
        log_error("No devices available\n");
        return 1;
    }

    std::vector<VkPhysicalDevice> physical_devices;
    physical_devices.resize(dev_count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, physical_devices.data()));

    // NOTE: A device defines types of queues that can perform specific work. 
    // Each queue type is called a queue family. Each queue family may have one
    // or more queues available for use. A queue family may support one or more
    // type of work.
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0], &queue_family_count, nullptr);
    assert(queue_family_count > 0);
    std::vector<VkQueueFamilyProperties> queue_family_properties;
    queue_family_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0], &queue_family_count, queue_family_properties.data());
    
    // Setup queue info
    uint32_t gr_queue_family_index = 0;
    bool found_graphics_queue = false;
    for (size_t i = 0; i < queue_family_properties.size(); ++i) {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            gr_queue_family_index = i;
            found_graphics_queue = true;
        }
    }
    if (!found_graphics_queue) {
        log_error("Unable to find a graphics queue on device\n");
    }

    float queue_priorities[1] = { 0.0 };
    VkDeviceQueueCreateInfo gr_queue_create_info = {};
    gr_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    gr_queue_create_info.pNext = nullptr;
    gr_queue_create_info.queueCount = 1;
    gr_queue_create_info.pQueuePriorities = queue_priorities;
    gr_queue_create_info.queueFamilyIndex = gr_queue_family_index;

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

    VkDevice device;
    VK_CHECK(vkCreateDevice(physical_devices[0], &device_info, nullptr, &device));

    // Allocate command buffer pools
    //
    // NOTE: Need one pool for each type of queue being used.
    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext = nullptr;
    cmd_pool_create_info.queueFamilyIndex = gr_queue_family_index;
    cmd_pool_create_info.flags = 0;
    VkCommandPool gr_cmd_pool = {};
    VK_CHECK(vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &gr_cmd_pool));

    // Create command buffers
    //
    VkCommandBufferAllocateInfo gr_cmd_buf_alloc_info = {};
    gr_cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    gr_cmd_buf_alloc_info.pNext = nullptr;
    gr_cmd_buf_alloc_info.commandPool = gr_cmd_pool;
    gr_cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    gr_cmd_buf_alloc_info.commandBufferCount = 1;
    VkCommandBuffer gr_cmd_buf = {};
    VK_CHECK(vkAllocateCommandBuffers(device, &gr_cmd_buf_alloc_info, &gr_cmd_buf));

    // Cleanup
    //
    vkFreeCommandBuffers(device, gr_cmd_pool, gr_cmd_buf_alloc_info.commandBufferCount, &gr_cmd_buf);
    vkDestroyCommandPool(device, gr_cmd_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    return 0;
}
