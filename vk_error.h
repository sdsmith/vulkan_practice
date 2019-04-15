#pragma once

#define NOMINMAX 1
#include <vulkan/vulkan.h>
#include <Windows.h>
#include <WinBase.h>

template<typename... Args>
void log_error(char const* format, Args... args)
{
    // TODO: detect when building in VS IDE vs all other cases
    constexpr size_t buf_size = 1024;
    char buf[buf_size];
    snprintf(buf, buf_size, format, args...);
    OutputDebugStringA(buf);
}

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

#define CONCAT2(A, B) A ## B
#define CONCAT(A, B) CONCAT2(A, B)
#define UNIQUE_IDENT(Ident) CONCAT(Ident, __COUNTER__)

#define VK_CHECK2(Expression, Result_Ident)              \
    do {                                                 \
        VkResult Result_Ident = (Expression);            \
        if (Result_Ident != VkResult::VK_SUCCESS) {      \
            log_error("%s:%d: %s\n", __FILE__, __LINE__, get_vk_error_msg(Result_Ident)); \
            return 1;                                    \
        }                                                \
    } while(0)
#define VK_CHECK(Expression) VK_CHECK2(Expression, UNIQUE_IDENT(vk_result))

