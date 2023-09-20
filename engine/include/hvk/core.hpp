#pragma once

#include <memory>
#include <source_location>
#include <utility>

#include <fmt/core.h>  // IWYU pragma: export
#include <fmt/format.h>  // IWYU pragma: export
#include <spdlog/spdlog.h>  // IWYU pragma: export
#include <vulkan/vk_enum_string_helper.h>

#include "hvk/types.hpp"  // IWYU pragma: export

namespace hvk {

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define XSTRINGIFY(arg) #arg
#define STRINGIFY(arg) XSTRINGIFY(arg)

#define HVK_ASSERT(expr, msg)                                            \
    do {                                                                 \
        if (!(expr)) {                                                   \
            spdlog::critical("FAILED ASSERTION: `{}`", STRINGIFY(expr)); \
            panic(msg);                                                  \
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

#define VKHPP_CHECK(expr, msg)                                                         \
    do {                                                                               \
        vk::Result result = (expr);                                                    \
        if (result != vk::Result::eSuccess) {                                          \
            spdlog::error("`{}` returned {}", STRINGIFY(expr), vk::to_string(result)); \
            throw std::runtime_error(msg);                                             \
        }                                                                              \
    } while (0)
// NOLINTEND(cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr u32 SYNC_TIMEOUT = 1000000000;

template<typename T>
[[noreturn]]
constexpr void panic(T&& msg, std::source_location location = {}) {
    spdlog::
        critical("PANIC: {} ({}:{})", std::forward<T>(msg), location.file_name(), location.line());
    abort();
}

template<typename T>
using Shared = std::shared_ptr<T>;

template<typename T>
using Unique = std::unique_ptr<T>;

}  // namespace hvk
