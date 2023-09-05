#define VMA_IMPLEMENTATION
#include "allocator.hpp"

namespace hc {

AllocatedBuffer create_staging_buffer(
    VmaAllocator allocator,
    vk::DeviceSize size
) {
    vk::BufferCreateInfo buf_info{};
    buf_info.setSize(size).setUsage(vk::BufferUsageFlagBits::eTransferSrc);
    auto vk_buf_info = static_cast<VkBufferCreateInfo>(buf_info);

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    AllocatedBuffer buf{};
    VK_CHECK(
        vmaCreateBuffer(
            allocator,
            &vk_buf_info,
            &alloc_info,
            &buf.buffer,
            &buf.allocation,
            nullptr
        ),
        "Failed to create staging buffer"
    );

    return buf;
}

}  // namespace hc
