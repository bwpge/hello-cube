#pragma once

#include <functional>

#include "core.hpp"
#include "allocator.hpp"

namespace hc {

class UploadContext {
public:
    explicit UploadContext(const vk::Device& device, u32 queue);

    UploadContext() = default;
    UploadContext(UploadContext&&) noexcept = default;
    UploadContext& operator=(const UploadContext&) = delete;
    UploadContext& operator=(UploadContext&&) noexcept = default;
    UploadContext(const UploadContext&) = delete;
    ~UploadContext() = default;

    void oneshot(
        vk::Device& device,
        vk::Queue& queue,
        std::function<void(vk::UniqueCommandBuffer&)>&& op
    );

    void copy_staged(
        vk::Device& device,
        vk::Queue& queue,
        AllocatedBuffer& src,
        AllocatedBuffer& dst,
        vk::DeviceSize size
    );

private:
    vk::UniqueFence _fence{};
    vk::UniqueCommandPool _pool{};
    vk::UniqueCommandBuffer _cmd{};
};

}  // namespace hc
