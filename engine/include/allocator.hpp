#pragma once

#include <vk_mem_alloc.h>

#include "core.hpp"

namespace hc {

struct AllocatedBuffer {
    VkBuffer buffer{};
    VmaAllocation allocation{};
};

struct AllocatedImage {
    VkImage image{};
    VmaAllocation allocation{};
};

AllocatedBuffer create_staging_buffer(
    VmaAllocator allocator,
    vk::DeviceSize size
);

}  // namespace hc
