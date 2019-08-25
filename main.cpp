#include "freetype_wrapper.h"
#include "platform.h"
#include "renderer.h"
#include "status.h"
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

static bool g_running = true;

void window_destroy_callback() {
    g_running = false;
}

int main()
{
    init_platform();

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

    uint32_t use_api_version = VK_API_VERSION_1_0;

    PFN_vkEnumerateInstanceVersion my_EnumerateInstanceVersion 
        = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
            vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));

    if (my_EnumerateInstanceVersion != nullptr) {
        uint32_t api_version;
        VK_CHECK(my_EnumerateInstanceVersion(&api_version));
        uint16_t api_major_ver = VK_VERSION_MAJOR(api_version);
        uint16_t api_minor_ver = VK_VERSION_MINOR(api_version);

        if (api_major_ver >= 1 && api_minor_ver >= 1) {
            // Vulkan 1.1 compatible
            use_api_version = VK_MAKE_VERSION(1, 1, 0);
        }
    }

    vulkan.app_info = {};
    vulkan.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vulkan.app_info.pNext = nullptr;
    vulkan.app_info.pApplicationName = app_name;
    vulkan.app_info.applicationVersion = app_ver;
    vulkan.app_info.pEngineName = engine_name;
    vulkan.app_info.engineVersion = engine_ver;
    vulkan.app_info.apiVersion = use_api_version;
    
    STATUS_CHECK(vulkan.create_instance());
    STATUS_CHECK(vulkan.setup_primary_physical_device());

    constexpr int window_width = 640;
    constexpr int window_height = 480;
    Rect window_rect = {};
    window_rect.x = CW_USEDEFAULT;
    window_rect.y = CW_USEDEFAULT;
    window_rect.width = window_width;
    window_rect.height = window_height;
    Window window = create_window(window_rect, window_destroy_callback);
    process_window_messages(window);

    STATUS_CHECK(vulkan.create_surface(window));
    STATUS_CHECK(vulkan.find_graphics_and_present_queue());
    STATUS_CHECK(vulkan.create_logical_device());
    STATUS_CHECK(vulkan.setup_device_queue());
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
    STATUS_CHECK(vulkan.setup_graphics_pipeline());

	//renderer.create_texture_atlas();

	Free_Type_Wrapper ft();
	ft.initialize();
	ft.load_font("resources/font/LiberationMono-Regular.ttf");
	ft.draw(0, 0, "Hello world!");

    const double desired_fps = 60;
    const double ms_per_frame = 1000.0 / desired_fps;
    uint32_t loop_num = 0;

    while (g_running) {
        const double time_start_ms = get_perf_counter_ms();

        process_window_messages(window);
        if (!g_running) { break; } // Break early since window has been destroyed

        Status res = vulkan.render();
        if (res != STATUS_OK) {
            log_error("Main loop: failed on loop %u\n", loop_num);
            STATUS_CHECK(res);
        }

        const double time_end_ms = get_perf_counter_ms();
        const double elapsed_frame_delta_ms = time_end_ms - time_start_ms;
        const double remaining_frame_time_ms = ms_per_frame - elapsed_frame_delta_ms;
        if (remaining_frame_time_ms > 0) {
            sleep(remaining_frame_time_ms);
        }

        loop_num++;
    }

    vulkan.cleanup();
    return 0;
}