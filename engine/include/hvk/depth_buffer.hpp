#pragma once

#include "hvk/core.hpp"
#include "hvk/allocator.hpp"
#include "hvk/vk_context.hpp"

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
    vk::Format format() const;
    vk::ImageView& image_view() noexcept;
    void destroy();

private:
    AllocatedImage _image{};
    vk::UniqueImageView _image_view{};
    vk::Format _format{vk::Format::eD32Sfloat};
};

}  // namespace hvk
