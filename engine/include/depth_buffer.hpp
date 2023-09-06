#pragma once

#include "core.hpp"
#include "allocator.hpp"
#include "vk_context.hpp"

namespace hvk {

class DepthBuffer {
public:
    DepthBuffer() = default;
    explicit DepthBuffer(vk::Extent2D extent);
    DepthBuffer(const DepthBuffer&) = delete;
    DepthBuffer(DepthBuffer&& other) noexcept;
    DepthBuffer& operator=(const DepthBuffer&) = delete;
    DepthBuffer& operator=(DepthBuffer&& rhs) noexcept;
    ~DepthBuffer();

    [[nodiscard]]
    inline vk::Format format() const {
        return _format;
    }

    inline vk::ImageView& image_view() noexcept {
        return _image_view.get();
    }

    void destroy();

private:
    AllocatedImage _image{};
    vk::UniqueImageView _image_view{};
    vk::Format _format{vk::Format::eD32Sfloat};
};

}  // namespace hvk
