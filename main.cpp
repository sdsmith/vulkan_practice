#include "vk_error.h"

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
    std::vector<VkPhysicalDevice> devices;
    devices.resize(dev_count);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &dev_count, devices.data()));

    // Destroy Vulkan instance
    //
    vkDestroyInstance(instance, nullptr);

    return 0;
}
