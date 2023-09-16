#pragma once

#include <memory>
#include <utility>

#include <spdlog/spdlog.h>
#include <vulkan/vk_enum_string_helper.h>

#include "hvk/types.hpp"

#define SYNC_TIMEOUT 1000000000

#define XSTRINGIFY(arg) #arg
#define STRINGIFY(arg) XSTRINGIFY(arg)

#define PANIC(msg)                                                      \
    do {                                                                \
        spdlog::critical("PANIC: {} ({}:{})", msg, __FILE__, __LINE__); \
        abort();                                                        \
    } while (0)

#define HVK_ASSERT(expr, msg)                                            \
    do {                                                                 \
        if (!(expr)) {                                                   \
            spdlog::critical("FAILED ASSERTION: `{}`", STRINGIFY(expr)); \
            PANIC(msg);                                                  \
        }                                                                \
    } while (0)

#define VK_CHECK(expr, msg)                \
    do {                                   \
        VkResult err = (expr);             \
        if (err) {                         \
            spdlog::error(                 \
                "`{}` returned {} ({})",   \
                STRINGIFY(expr),           \
                string_VkResult(err),      \
                static_cast<i64>(err)      \
            );                             \
            throw std::runtime_error(msg); \
        }                                  \
    } while (0)

#define VKHPP_CHECK(expr, msg)                                             \
    do {                                                                   \
        vk::Result result = (expr);                                        \
        if (result != vk::Result::eSuccess) {                              \
            spdlog::error(                                                 \
                "`{}` returned {}", STRINGIFY(expr), vk::to_string(result) \
            );                                                             \
            throw std::runtime_error(msg);                                 \
        }                                                                  \
    } while (0)

namespace hvk {

template <typename T>
using Shared = std::shared_ptr<T>;

template <typename T>
using Unique = std::unique_ptr<T>;

}  // namespace hvk
