#pragma once

#include <functional>

#include "hvk/core.hpp"
#include "hvk/allocator.hpp"
#include "hvk/vk_context.hpp"

namespace hvk {

class UploadContext {
public:
    explicit UploadContext(u32 queue);

    UploadContext() = default;
    UploadContext(UploadContext&&) noexcept = default;
    UploadContext& operator=(const UploadContext&) = delete;
    UploadContext& operator=(UploadContext&&) noexcept = default;
    UploadContext(const UploadContext&) = delete;
    ~UploadContext() = default;

    void oneshot(
        const vk::Queue& queue,
        std::function<void(const vk::UniqueCommandBuffer&)>&& op
    );

    void copy_staged(
        const vk::Queue& queue,
        AllocatedBuffer& src,
        AllocatedBuffer& dst,
        vk::DeviceSize size
    );

private:
    vk::UniqueFence _fence{};
    vk::UniqueCommandPool _pool{};
    vk::UniqueCommandBuffer _cmd{};
};

}  // namespace hvk
