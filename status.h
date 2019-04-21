#pragma once

#include "platform.h"
#include "types.h"

using Status = int;
static constexpr Status STATUS_OK = 0;

#define STATUS_CHECK2(Expression, Result_Ident) \
    do {                                        \
        Status Result_Ident = (Expression);     \
        if (Result_Ident != STATUS_OK) {        \
            log_error("%s:%d: %s\n", __FILE__, __LINE__, "status check failed"); \
            return Result_Ident;                \
        }                                       \
    } while(0)
#define STATUS_CHECK(Expression) STATUS_CHECK2(Expression, UNIQUE_IDENT(vk_result))
