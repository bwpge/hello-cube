#pragma once

#include "core.hpp"
#include "allocator.hpp"

namespace hc {

class UniformBufferObject {
public:
    UniformBufferObject() = default;
    UniformBufferObject(VmaAllocator allocator, vk::DeviceSize size);
    UniformBufferObject(const UniformBufferObject&) = delete;
    UniformBufferObject(UniformBufferObject&& rhs) noexcept;
    UniformBufferObject& operator=(const UniformBufferObject&) = delete;
    UniformBufferObject& operator=(UniformBufferObject&& rhs) noexcept;
    ~UniformBufferObject() = default;

    template <typename T>
    void update(T* src) {
        auto* new_data = static_cast<void*>(src);
        memcpy(_data, new_data, sizeof(T));
    }

    template <>
    void update(void*) = delete;

    [[nodiscard]]
    inline vk::Buffer buffer() const {
        return static_cast<vk::Buffer>(_buf.buffer);
    }

    void destroy();

private:
    VmaAllocator _allocator{};
    AllocatedBuffer _buf{};
    void* _data{};
};

}  // namespace hc
