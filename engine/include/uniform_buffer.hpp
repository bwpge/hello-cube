#pragma once

#include "core.hpp"
#include "allocator.hpp"
#include "vk_context.hpp"

namespace hvk {

class UniformBuffer {
public:
    explicit UniformBuffer(vk::DeviceSize size);
    explicit UniformBuffer(vk::DeviceSize range, vk::DeviceSize size);

    UniformBuffer() = default;
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& other) noexcept;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer& operator=(UniformBuffer&& rhs) noexcept;
    ~UniformBuffer();

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
    inline bool is_mapped() const {
        return static_cast<bool>(
            _mem_props & vk::MemoryPropertyFlagBits::eHostVisible
        );
    }

    template <typename T>
    void update(T* src) {
        auto* new_data = static_cast<void*>(src);
        auto size = sizeof(T);
        update_impl(_data, new_data, size);
    }

    template <>
    void update(void*) = delete;

    template <typename T>
    void update_indexed(T* src, usize dyn_index) {
        auto size = sizeof(T);
        auto offset = dyn_index * UniformBuffer::pad_alignment(size);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto* data = static_cast<void*>(static_cast<char*>(_data) + offset);
        auto* new_data = static_cast<void*>(src);
        update_impl(data, new_data, size);
    }

    template <>
    void update_indexed(void*, usize) = delete;

    [[nodiscard]]
    inline vk::Buffer buffer() const {
        return static_cast<vk::Buffer>(_buf.buffer);
    }

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
