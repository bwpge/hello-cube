#pragma once

#include "hvk/core.hpp"
#include "hvk/allocator.hpp"
#include "hvk/vk_context.hpp"

namespace hvk {

class Buffer {
public:
    explicit Buffer(vk::DeviceSize size);
    explicit Buffer(vk::DeviceSize range, vk::DeviceSize size);

    Buffer() = default;
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&& rhs) noexcept;
    ~Buffer();

    [[nodiscard]]
    static usize pad_alignment(usize size) {
        // https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
        const auto properties = VulkanContext::gpu().getProperties();
        usize min_alignment = properties.limits.minUniformBufferOffsetAlignment;
        usize aligned = size;
        if (min_alignment > 0) {
            aligned = (aligned + min_alignment - 1) & ~(min_alignment - 1);
        }
        return aligned;
    }

    [[nodiscard]]
    bool is_mapped() const;

    template <typename T>
    void update(T* src) {
        auto* new_data = static_cast<void*>(src);
        auto size = sizeof(T);
        update_impl(_data, new_data, size);
    }

    template <>
    void update(void*) = delete;

    template <typename T>
    void update_indexed(T* src, usize index) {
        auto size = sizeof(T);
        auto offset = index * Buffer::pad_alignment(size);
        HVK_ASSERT(
            offset < _buf.size,
            "Index offset must point to an address within the buffer memory"
        );

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto* data = static_cast<void*>(static_cast<char*>(_data) + offset);
        auto* new_data = static_cast<void*>(src);
        update_impl(data, new_data, size);
    }

    template <>
    void update_indexed(void*, usize) = delete;

    [[nodiscard]]
    vk::Buffer buffer() const;
    [[nodiscard]]
    usize range() const;
    [[nodiscard]]
    usize size() const;
    [[nodiscard]]
    u32 dyn_offset(usize idx) const;
    [[nodiscard]]
    vk::DescriptorBufferInfo descriptor_buffer_info(usize offset = 0) const;

    void destroy();

private:
    void allocate_impl(usize size);
    void update_impl(void* dst, void* src, usize size) const;

    AllocatedBuffer _buf{};
    usize _range{};
    void* _data{};
    vk::MemoryPropertyFlags _mem_props{};
};

}  // namespace hvk
