#pragma once

#include <vk_mem_alloc.h>

namespace hc {

struct AllocatedBuffer {
    VkBuffer buffer{};
    VmaAllocation allocation{};
};

struct AllocatedImage {
    VkImage image{};
    VmaAllocation allocation{};
};

}  // namespace hc
