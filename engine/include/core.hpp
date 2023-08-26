#pragma once

#include <spdlog/spdlog.h>

#include "types.hpp"

#define _STRINGIFY(arg) #arg
#define STRINGIFY(arg) _STRINGIFY(arg)

#define PANIC(msg)                                                      \
    do {                                                                \
        spdlog::critical("PANIC: {} ({}:{})", msg, __FILE__, __LINE__); \
        throw std::runtime_error(msg);                                  \
    } while (0)

#define HC_ASSERT(expr, msg)                                         \
    do {                                                             \
        if (!(expr)) {                                               \
            spdlog::error("FAILED ASSERTION `{}`", STRINGIFY(expr)); \
            PANIC(msg);                                              \
        }                                                            \
    } while (0)
