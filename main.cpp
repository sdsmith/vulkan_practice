#include "platform.h"
#include "renderer.h"
#include "status.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

int main()
{
    Vulkan_Instance_Info vulkan = {};

    constexpr char const* app_name = "Vulkan Practice";
    constexpr uint32_t app_ver = 1;
    constexpr char const* engine_name = "Vulkan Practice Engine";
    constexpr uint32_t engine_ver = 1;
 
#if defined(_DEBUG)
    vulkan.instance_layer_names.push_back("VK_LAYER_LUNARG_standard_validation");
    vulkan.instance_layer_names.push_back("VK_LAYER_LUNARG_parameter_validation");
#endif

    // Want the Window System Integration (WSI) extensions.
    // - requires general surface extension
    vulkan.instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    vulkan.instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
#   error Unsupported platform
#endif

    vulkan.device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    
    vulkan.app_info = {};
    vulkan.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vulkan.app_info.pNext = nullptr;
    vulkan.app_info.pApplicationName = app_name;
    vulkan.app_info.applicationVersion = app_ver;
    vulkan.app_info.pEngineName = engine_name;
    vulkan.app_info.engineVersion = engine_ver;
    vulkan.app_info.apiVersion = VK_API_VERSION_1_0;
    
    STATUS_CHECK(vulkan.create_instance());
    STATUS_CHECK(vulkan.setup_primary_physical_device());

    constexpr int window_width = 640;
    constexpr int window_height = 480;
    Rect window_rect = {};
    window_rect.x = CW_USEDEFAULT;
    window_rect.y = CW_USEDEFAULT;
    window_rect.width = window_width;
    window_rect.height = window_height;
    Window window = create_window(window_rect);
    process_window_messages(window);

    STATUS_CHECK(vulkan.create_surface(window));
    STATUS_CHECK(vulkan.find_graphics_and_present_queue());
    STATUS_CHECK(vulkan.create_logical_device());
    STATUS_CHECK(vulkan.create_command_pool());
    STATUS_CHECK(vulkan.create_command_buffer());

    constexpr uint32_t desired_buf_strategy = 2;
    STATUS_CHECK(vulkan.setup_swapchain(desired_buf_strategy, window_width, window_height));
	STATUS_CHECK(vulkan.setup_depth_buffer());
    STATUS_CHECK(vulkan.setup_model_view_projection());
    STATUS_CHECK(vulkan.setup_uniform_buffer());
    STATUS_CHECK(vulkan.setup_pipeline());
    STATUS_CHECK(vulkan.setup_render_pass());
	STATUS_CHECK(vulkan.setup_shaders());
	STATUS_CHECK(vulkan.setup_framebuffer());
	STATUS_CHECK(vulkan.setup_vertex_buffer());

    vulkan.cleanup();
    return 0;
}
