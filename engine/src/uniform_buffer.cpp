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
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VK_CHECK(
        vmaCreateBuffer(
            _allocator,
            &vk_info,
            &alloc_info,
            &_buf.buffer,
            &_buf.allocation,
            nullptr
        ),
        "Failed to allocate uniform buffer object"
    );

    VK_CHECK(
        vmaMapMemory(_allocator, _buf.allocation, &_data),
        "Failed to map memory allocation"
    );
}

UniformBufferObject::UniformBufferObject(UniformBufferObject&& rhs) noexcept {
    destroy();
    std::swap(_allocator, rhs._allocator);
    std::swap(_buf, rhs._buf);
    std::swap(_data, rhs._data);
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
        if (_buf.allocation) {
            vmaUnmapMemory(_allocator, _buf.allocation);
        }
        vmaDestroyBuffer(_allocator, _buf.buffer, _buf.allocation);
    }
}

}  // namespace hc
