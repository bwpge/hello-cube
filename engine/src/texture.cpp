#define STB_IMAGE_IMPLEMENTATION
#include "hvk/texture.hpp"

namespace hvk {

ImageResource::ImageResource(ImageResource&& other) noexcept {
    std::swap(_image, other._image);
}

ImageResource& ImageResource::operator=(ImageResource&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    destroy();
    std::swap(_image, rhs._image);

    return *this;
}

ImageResource::~ImageResource() {
    destroy();
}

vk::UniqueImageView ImageResource::create_image_view(
    vk::Format format,
    vk::ImageAspectFlags aspect_mask
) const {
    vk::ImageViewCreateInfo create_info{};
    create_info.setImage(_image.image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format);
    create_info.subresourceRange.setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setAspectMask(aspect_mask);

    return VulkanContext::device().createImageViewUnique(create_info);
}

bool ImageResource::has_alpha() const {
    return _has_alpha;
}

void ImageResource::destroy() {
    VulkanContext::allocator().destroy(_image);
}

Texture::Texture(
    const ImageResource& resource,
    vk::Filter filter,
    vk::SamplerAddressMode mode
)
    : _has_alpha{resource.has_alpha()},
      _view{resource.create_image_view()}  //
{
    vk::SamplerCreateInfo info{};
    info.setMagFilter(filter)
        .setMinFilter(filter)
        .setAddressModeU(mode)
        .setAddressModeV(mode)
        .setAddressModeW(mode);
    _sampler = VulkanContext::device().createSamplerUnique(info);
}

const vk::Sampler& Texture::sampler() const {
    return _sampler.get();
}

const vk::ImageView& Texture::image_view() const {
    return _view.get();
}

vk::DescriptorImageInfo Texture::create_image_info(vk::ImageLayout layout
) const {
    vk::DescriptorImageInfo info{};
    info.setSampler(sampler())
        .setImageView(image_view())
        .setImageLayout(layout);
    return info;
}

}  // namespace hvk
