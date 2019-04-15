#include "vk_error.h"

#include <cassert>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

constexpr char const* window_class_name = "VulkanPractice";
constexpr char const* window_name = "Vulkan Practive";

LRESULT CALLBACK window_proc_callback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, w_param, l_param);
    }

    return 0;
}

struct Rect
{
    int x;
    int y;
    int width;
    int height;
};

#ifdef _WIN32
struct Window
{
    HINSTANCE h_instance;
    HWND h_window;
};

Window create_window(const Rect& window_rect)
{
    Window window = {};
    window.h_instance = GetModuleHandle(nullptr);

    // Register the window class
    WNDCLASSEXA window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.style = 0;
    window_class.lpfnWndProc = window_proc_callback;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = window.h_instance;
    window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszMenuName = nullptr;
    window_class.lpszClassName = window_class_name;
    window_class.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    ATOM window_class_atom = RegisterClassExA(&window_class);
    if (!window_class_atom) {
        assert(!"Unable to register window class");
    }

    // Create the window
    window.h_window = CreateWindowA(window_class_name, window_name, WS_TILEDWINDOW, window_rect.x, window_rect.y, window_rect.width, window_rect.height, nullptr, nullptr, window.h_instance, nullptr);
    if (!window.h_window) {
        assert(!"Unable to create window");
    }

    ShowWindow(window.h_window, SW_SHOW);
    UpdateWindow(window.h_window);

    return window;
}

int process_window_messages(Window const& window)
{
    int num_msgs = 0;
    MSG window_msg;
    while (PeekMessageA(&window_msg, window.h_window, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&window_msg);
        DispatchMessage(&window_msg); // call the window proc callback
        ++num_msgs;
    }

    return num_msgs;
}
#endif

int main()
{
    constexpr char const* app_name = "Vulkan Practice";
    constexpr uint32_t app_ver = 1;
    constexpr char const* engine_name = "Vulkan Practice Engine";
    constexpr uint32_t engine_ver = 1;

    std::vector<char const*> layers;
#if defined(_DEBUG)
    layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif

    std::vector<char const*> instance_extension_names;

    // Want the Window System Integration (WSI) extensions.
    // - requires general surface extension
    instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
#   error Unsupported platform
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
    inst_info.enabledExtensionCount = static_cast<uint32_t>(instance_extension_names.size());
    inst_info.ppEnabledExtensionNames = instance_extension_names.data();
    inst_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    inst_info.ppEnabledLayerNames = layers.data();

    // Create Vulkan instance
    //
    VkInstance vk_instance;
    VK_CHECK(vkCreateInstance(&inst_info, nullptr, &vk_instance));

    // Setup physical device
    //
    uint32_t dev_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(vk_instance, &dev_count, nullptr));
    if (dev_count == 0) {
        log_error("No devices available\n");
        return 1;
    }

    std::vector<VkPhysicalDevice> physical_devices;
    physical_devices.resize(dev_count);
    VK_CHECK(vkEnumeratePhysicalDevices(vk_instance, &dev_count, physical_devices.data()));

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

    // Create window
    //
    Rect window_rect = {};
    window_rect.x = CW_USEDEFAULT;
    window_rect.y = CW_USEDEFAULT;
    window_rect.width = 640;
    window_rect.height = 480;

    Window window = create_window(window_rect);
    process_window_messages(window);

    // Construct surface
    VkSurfaceKHR surface;
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = nullptr;
    surface_create_info.hinstance = window.h_instance;
    surface_create_info.hwnd = window.h_window;
    VK_CHECK(vkCreateWin32SurfaceKHR(vk_instance, &surface_create_info, nullptr, &surface));
#else
#   error Unsupported platform
#endif

    // Query queues that support presenting surfaces
    std::vector<VkBool32> queue_family_supports_present(queue_family_properties.size(), false);
    for (uint32_t i = 0; i < queue_family_properties.size(); ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[0], i, surface, &queue_family_supports_present[i]);
    }

    // Find graphics and present queue, perferably one that supports both
    uint32_t gr_queue_family_index = std::numeric_limits<uint32_t>::max();
    uint32_t present_queue_family_index = std::numeric_limits<uint32_t>::max();

    for (size_t i = 0; i < queue_family_properties.size(); ++i) {
        bool const supports_graphics = queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        if (supports_graphics) {
            gr_queue_family_index = i;
        }

        if (queue_family_supports_present[i]) {
            present_queue_family_index = i;
        }

        if (gr_queue_family_index == present_queue_family_index) {
            // found a queue that supports both grpahics and present, exit
            break;
        }
    }

    if (gr_queue_family_index == std::numeric_limits<uint32_t>::max()) {
        log_error("Unable to find a graphics queue on device\n");
        return 1;
    }
    if (present_queue_family_index == std::numeric_limits<uint32_t>::max()) {
        log_error("Unable to find a present queue on device\n");
        return 1;
    }

    // Setup queue infos
    // TODO: setup the present queue (may be the same as the graphics queue)
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

    // Setup swapchain
    //
    // TODO: vkGetPhysicalDeviceSurfaceCapabilitiesKHR()
    //       vkGetPhysicalDeviceSurfacePresentModesKHR();


    for (size_t i = 0; i < queue_family_count; ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, )
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = nullptr;
    swapchain_create_info.surface = surface;
    swapchain_create_info.imageFormat = ; // TODO: pick the image format based on those available to the display (VkSurfaceFormatKHR)
    swapchain_create_info.minImageCount = num_swapchain_images; // Set to the buffering strategy (double or triple buffer)
    swapchain_create_info.imageExtent.width = window.width;
    swapchain_create_info.imageExtent.height = window.height;
    swapchain_create_info.preTransform = ;
    swapchain_create_info.presentMode = ;

    // Cleanup
    //
    vkFreeCommandBuffers(device, gr_cmd_pool, gr_cmd_buf_alloc_info.commandBufferCount, &gr_cmd_buf);
    vkDestroyCommandPool(device, gr_cmd_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(vk_instance, nullptr);

    return 0;
}
