#include "depth_buffer.hpp"

namespace hc {

DepthBuffer::DepthBuffer(vk::Extent2D extent) {
    auto& allocator = VulkanContext::allocator();

    vk::ImageCreateInfo ici{};
    ici.setImageType(vk::ImageType::e2D)
        .setExtent(vk::Extent3D{extent.width, extent.height, 1})
        .setFormat(_format)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setMipLevels(1)
        .setArrayLayers(1)
        .setTiling(vk::ImageTiling::eOptimal);

    auto image = allocator.create_image(
        ici,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    );
    allocator.destroy(_image);
    std::swap(_image, image);

    vk::ImageViewCreateInfo ivci{};
    ivci.setViewType(vk::ImageViewType::e2D)
        .setImage(vk::Image{_image.image})
        .setFormat(_format);
    ivci.subresourceRange.setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setAspectMask(vk::ImageAspectFlagBits::eDepth);

    _image_view = VulkanContext::device().createImageViewUnique(ivci);
}

void DepthBuffer::destroy() {
    VulkanContext::allocator().destroy(_image);
}

}  // namespace hc
