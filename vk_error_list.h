#include <vulkan/vulkan.h>

#ifndef ERROR_CODE
#   error ERROR_CODE(VkResult, char const*) must be defined before including this file.
#endif

// Success codes
ERROR_CODE(VkResult::VK_SUCCESS, "Command successfully completed")
ERROR_CODE(VkResult::VK_NOT_READY, "A fence or query has not yet completed")
ERROR_CODE(VkResult::VK_TIMEOUT, "A wait operation has not completed in the specified time")
ERROR_CODE(VkResult::VK_EVENT_SET, "An event is signaled")
ERROR_CODE(VkResult::VK_EVENT_RESET, "An event is unsignaled")
ERROR_CODE(VkResult::VK_INCOMPLETE, "A return array was too small for the result")
ERROR_CODE(VkResult::VK_SUBOPTIMAL_KHR, "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.")

// Error codes
ERROR_CODE(VkResult::VK_ERROR_OUT_OF_HOST_MEMORY, "A host memory allocation has failed.")
ERROR_CODE(VkResult::VK_ERROR_OUT_OF_DEVICE_MEMORY, "A device memory allocation has failed.")
ERROR_CODE(VkResult::VK_ERROR_INITIALIZATION_FAILED, "Initialization of an object could not be completed for implementation - specific reasons.")
ERROR_CODE(VkResult::VK_ERROR_DEVICE_LOST, "The logical or physical device has been lost.")
ERROR_CODE(VkResult::VK_ERROR_MEMORY_MAP_FAILED, "Mapping of a memory object has failed.")
ERROR_CODE(VkResult::VK_ERROR_LAYER_NOT_PRESENT, "A requested layer is not present or could not be loaded.")
ERROR_CODE(VkResult::VK_ERROR_EXTENSION_NOT_PRESENT, "A requested extension is not supported.")
ERROR_CODE(VkResult::VK_ERROR_FEATURE_NOT_PRESENT, "A requested feature is not supported.")
ERROR_CODE(VkResult::VK_ERROR_INCOMPATIBLE_DRIVER, "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation - specific reasons.")
ERROR_CODE(VkResult::VK_ERROR_TOO_MANY_OBJECTS, "Too many objects of the type have already been created.")
ERROR_CODE(VkResult::VK_ERROR_FORMAT_NOT_SUPPORTED, "A requested format is not supported on this device.")
ERROR_CODE(VkResult::VK_ERROR_FRAGMENTED_POOL, "A pool allocation has failed due to fragmentation of the pool’s memory.This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation.")
ERROR_CODE(VkResult::VK_ERROR_SURFACE_LOST_KHR, "A surface is no longer available.")
ERROR_CODE(VkResult::VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.")
ERROR_CODE(VkResult::VK_ERROR_OUT_OF_DATE_KHR, "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.")
ERROR_CODE(VkResult::VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.")
ERROR_CODE(VkResult::VK_ERROR_INVALID_SHADER_NV, "One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.")
ERROR_CODE(VkResult::VK_ERROR_OUT_OF_POOL_MEMORY, "A pool memory allocation has failed.")
ERROR_CODE(VkResult::VK_ERROR_INVALID_EXTERNAL_HANDLE, "An external handle is not a valid handle of the specified type.")
ERROR_CODE(VkResult::VK_ERROR_FRAGMENTATION_EXT, "A descriptor pool creation has failed due to fragmentation.")
ERROR_CODE(VkResult::VK_ERROR_INVALID_DEVICE_ADDRESS_EXT, "A buffer creation failed because the requested address is not available.")
//ERROR_CODE(VkResult::VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full - screen access. This may occur due to implementation - dependent reasons, outside of the application’s control.")
