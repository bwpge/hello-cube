#include "uniform_buffer.hpp"

namespace hc {

UniformBufferObject::UniformBufferObject(
    VmaAllocator allocator,
    vk::DeviceSize size
)
    : _allocator{allocator} {
    vk::BufferCreateInfo info{};
    info.setSize(size).setUsage(vk::BufferUsageFlagBits::eUniformBuffer);
    auto vk_info = static_cast<VkBufferCreateInfo>(info);

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocation_info{};
    VK_CHECK(
        vmaCreateBuffer(
            _allocator,
            &vk_info,
            &alloc_info,
            &_buf.buffer,
            &_buf.allocation,
            &allocation_info
        ),
        "Failed to allocate uniform buffer object"
    );
    _data = allocation_info.pMappedData;
    HC_ASSERT(
        _data,
        "Persistent memory mapping failed (mapped data was a null pointer)"
    );

    VkMemoryPropertyFlags flags{};
    vmaGetAllocationMemoryProperties(_allocator, _buf.allocation, &flags);
    HC_ASSERT(
        flags != 0,
        "`vmaGetAllocationMemoryProperties` failed to get property flags"
    );
    _mem_props = vk::MemoryPropertyFlags{flags};
}

UniformBufferObject::UniformBufferObject(UniformBufferObject&& other) noexcept {
    destroy();
    std::swap(_allocator, other._allocator);
    std::swap(_buf, other._buf);
    std::swap(_data, other._data);
    std::swap(_mem_props, other._mem_props);
}

UniformBufferObject& UniformBufferObject::operator=(UniformBufferObject&& rhs
) noexcept {
    if (this == &rhs) {
        return *this;
    }

    destroy();
    std::swap(_allocator, rhs._allocator);
    std::swap(_buf, rhs._buf);
    std::swap(_data, rhs._data);
    std::swap(_mem_props, rhs._mem_props);

    return *this;
}

void UniformBufferObject::destroy() {
    spdlog::trace(
        "Destroying UBO: allocator={}, buf={}, allocation={}",
        spdlog::fmt_lib::ptr(_allocator),
        spdlog::fmt_lib::ptr(_buf.buffer),
        spdlog::fmt_lib::ptr(_buf.allocation)
    );

    if (_allocator) {
        vmaDestroyBuffer(_allocator, _buf.buffer, _buf.allocation);
    }
}

}  // namespace hc
