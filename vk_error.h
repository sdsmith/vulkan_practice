#pragma once

#include "platform.h"
#include "status.h"
#include "types.h"
#include <vulkan/vulkan.h>

constexpr char const* get_vk_error_msg(VkResult result)
{
    switch (result) {
#define ERROR_CODE(Result, Msg) case Result: return Msg;
#   include "vk_error_list.h"
#undef ERROR_CODE

    default:
        return "An unknown error occured.";
    }
}

#define VK_CHECK2(Expression, Result_Ident)              \
    do {                                                 \
        VkResult Result_Ident = (Expression);            \
        if (Result_Ident != VkResult::VK_SUCCESS) {      \
            log_error("%s:%d: %s\n", __FILE__, __LINE__, get_vk_error_msg(Result_Ident)); \
            return !STATUS_OK;                           \
        }                                                \
    } while(0)
#define VK_CHECK(Expression) VK_CHECK2(Expression, UNIQUE_IDENT(vk_result))
