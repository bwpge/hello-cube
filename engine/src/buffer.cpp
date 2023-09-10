#include "hvk/buffer.hpp"

namespace hvk {

Buffer::Buffer(vk::DeviceSize size) : _range{size} {
    allocate_impl(size);
}

Buffer::Buffer(vk::DeviceSize range, vk::DeviceSize size) : _range{range} {
    allocate_impl(size);
}

Buffer::~Buffer() {
    destroy();
}

bool Buffer::is_mapped() const {
    return static_cast<bool>(
        _mem_props & vk::MemoryPropertyFlagBits::eHostVisible
    );
}

vk::Buffer Buffer::buffer() const {
    return static_cast<vk::Buffer>(_buf.buffer);
}

usize Buffer::range() const {
    return _range;
}

usize Buffer::size() const {
    return _buf.size;
}

u32 Buffer::dyn_offset(usize idx) const {
    return static_cast<u32>(idx * Buffer::pad_alignment(_range));
}

vk::DescriptorBufferInfo Buffer::descriptor_buffer_info(usize offset) const {
    vk::DescriptorBufferInfo info{};
    info.setBuffer(buffer()).setRange(_range).setOffset(offset);
    return info;
}

Buffer::Buffer(Buffer&& other) noexcept {
    destroy();
    std::swap(_buf, other._buf);
    std::swap(_range, other._range);
    std::swap(_data, other._data);
    std::swap(_mem_props, other._mem_props);
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    destroy();
    std::swap(_buf, rhs._buf);
    std::swap(_range, rhs._range);
    std::swap(_data, rhs._data);
    std::swap(_mem_props, rhs._mem_props);

    return *this;
}

void Buffer::destroy() {
    VulkanContext::allocator().destroy(_buf);
}

void Buffer::allocate_impl(usize size) {
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

void Buffer::update_impl(void* dst, void* src, usize size) const {
    if (is_mapped()) {
        memcpy(dst, src, size);
    } else {
        // TODO(bwpge): implement transfer
        PANIC("TODO: Implement transfer for non-mappable memory");
    }
}

}  // namespace hvk
