#pragma once

#include "glm/glm.hpp"
#include "vk_error.h"
#include <vulkan/vulkan.h>
#include <vector>

struct Swapchain_Buffer {
    VkImage image;
    VkImageView view;
};

struct Depth_Buffer {
	VkImage image;
	VkImageView view;
	VkDeviceMemory mem;
};

struct Uniform_Data {
    VkBuffer buf;
    VkDeviceMemory mem;
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
	VkExtent2D swapchain_extent;

	Depth_Buffer depth_buf;

    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 clip;
    glm::mat4 mvp;

    Uniform_Data uniform_data;
    
    std::vector<VkDescriptorSetLayout> desc_set_layouts;
    VkPipelineLayout pipeline_layout;

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
			VkPhysicalDeviceMemoryProperties memory_properties;

            struct Queue
            {
                uint32_t gr_family_index;
                uint32_t present_family_index;
            } queue;
        } primary;
    } system;

    Status create_instance();
    Status setup_primary_physical_device();
    Status find_graphics_and_present_queue();
    Status create_logical_device();
    Status create_command_pool();
    Status create_command_buffer();

#ifdef _WIN32
    Status create_surface(Window const& window);
#else
#   error Unsupported platform
#endif

    Status setup_swapchain(uint32_t num_buf_frames, uint32_t image_width, uint32_t image_height);
    bool memory_type_from_properties(uint32_t type_bits, VkFlags requirements_mask, uint32_t* type_index);
    Status setup_depth_buffer();
    Status setup_model_view_projection();
    Status setup_uniform_buffer();
    Status setup_pipeline_layout();

    void cleanup();
};
