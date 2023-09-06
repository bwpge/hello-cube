#include "uniform_buffer.hpp"

namespace hvk {

UniformBufferObject::UniformBufferObject(vk::DeviceSize size) {
    auto& allocator = VulkanContext::allocator();

    VmaAllocationInfo allocation_info{};
    auto buffer = allocator.create_buffer(
        size,
        vk::BufferUsageFlagBits::eUniformBuffer,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_AUTO,
        &allocation_info
    );
    allocator.destroy(_buf);
    _buf = buffer;

    _data = allocation_info.pMappedData;
    HVK_ASSERT(
        _data, "Persistent memory mapping failed (mapping was a null pointer)"
    );
    _mem_props = allocator.get_memory_property_flags(_buf);
}

UniformBufferObject::~UniformBufferObject() {
    destroy();
}

UniformBufferObject::UniformBufferObject(UniformBufferObject&& other) noexcept {
    destroy();
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
    std::swap(_buf, rhs._buf);
    std::swap(_data, rhs._data);
    std::swap(_mem_props, rhs._mem_props);

    return *this;
}

void UniformBufferObject::destroy() {
    VulkanContext::allocator().destroy(_buf);
}

}  // namespace hvk
