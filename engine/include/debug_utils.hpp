#pragma once

#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>

namespace hc {

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    VkDebugUtilsMessengerCallbackDataEXT const* cb_data,
    void* user_data
);

}  // namespace hc
