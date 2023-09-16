#pragma once

#include <filesystem>

#include <vulkan/vulkan.hpp>
#include <stb_image.h>

#include "hvk/core.hpp"
#include "hvk/upload_context.hpp"

namespace hvk {

void transfer_image(const vk::UniqueCommandBuffer& cmd);

class ImageResource {
public:
    ImageResource() = default;
    ImageResource(const std::filesystem::path& path, UploadContext& ctx);
    ImageResource(const ImageResource&) = delete;
    ImageResource(ImageResource&& other) noexcept;
    ImageResource& operator=(const ImageResource&) = delete;
    ImageResource& operator=(ImageResource&& rhs) noexcept;
    ~ImageResource();

    [[nodiscard]]
    vk::UniqueImageView create_image_view(
        vk::Format format = vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor
    ) const;
    [[nodiscard]]
    bool has_alpha() const;

private:
    void destroy();

    AllocatedImage _image{};
    bool _has_alpha{false};
};

class Texture {
public:
    Texture() = default;
    explicit Texture(
        const ImageResource& resource,
        vk::Filter filter,
        vk::SamplerAddressMode mode
    );

    [[nodiscard]]
    const vk::Sampler& sampler() const;
    [[nodiscard]]
    const vk::ImageView& image_view() const;
    [[nodiscard]]
    vk::DescriptorImageInfo create_image_info(
        vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal
    ) const;

private:
    bool _has_alpha{};
    vk::UniqueSampler _sampler{};
    vk::UniqueImageView _view{};
};

}  // namespace hvk
