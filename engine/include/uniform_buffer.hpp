#pragma once

#include "core.hpp"
#include "allocator.hpp"
#include "vk_context.hpp"

namespace hc {

class UniformBufferObject {
public:
    UniformBufferObject() = default;
    explicit UniformBufferObject(vk::DeviceSize size);
    UniformBufferObject(const UniformBufferObject&) = delete;
    UniformBufferObject(UniformBufferObject&& other) noexcept;
    UniformBufferObject& operator=(const UniformBufferObject&) = delete;
    UniformBufferObject& operator=(UniformBufferObject&& rhs) noexcept;
    ~UniformBufferObject() = default;

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

        if (is_mapped()) {
            memcpy(_data, new_data, size);
        } else {
            // TODO(bwpge): implement transfer
            PANIC("TODO: Implement transfer for non-mappable memory");
        }
    }

    template <>
    void update(void*) = delete;

    [[nodiscard]]
    inline vk::Buffer buffer() const {
        return static_cast<vk::Buffer>(_buf.buffer);
    }

    void destroy();

private:
    AllocatedBuffer _buf{};
    void* _data{};
    vk::MemoryPropertyFlags _mem_props{};
};

}  // namespace hc
