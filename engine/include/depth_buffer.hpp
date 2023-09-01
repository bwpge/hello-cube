#pragma once

#include "allocator.hpp"
#include "core.hpp"

namespace hc {

class DepthBuffer {
public:
    DepthBuffer() = default;
    explicit DepthBuffer(
        VmaAllocator allocator,
        const vk::Device& device,
        vk::Extent2D extent
    );
    DepthBuffer(const DepthBuffer&) = delete;
    DepthBuffer(DepthBuffer&&) noexcept = default;
    DepthBuffer& operator=(const DepthBuffer&) = delete;
    DepthBuffer& operator=(DepthBuffer&&) noexcept = default;
    ~DepthBuffer() = default;

    [[nodiscard]]
    inline vk::Format format() const {
        return _format;
    }

    inline vk::ImageView& image_view() noexcept {
        return _image_view.get();
    }

    void destroy();

private:
    void destroy_image(AllocatedImage& buf);

    AllocatedImage _image{};
    vk::UniqueImageView _image_view{};
    VmaAllocator _allocator{};
    vk::Format _format{vk::Format::eD32Sfloat};
};

}  // namespace hc
