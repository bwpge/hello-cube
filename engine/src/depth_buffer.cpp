#include "depth_buffer.hpp"

namespace hc {

DepthBuffer::DepthBuffer(
    VmaAllocator allocator,
    const vk::Device& device,
    vk::Extent2D extent
)
    : _allocator{allocator} {
    vk::ImageCreateInfo ici{};
    ici.setImageType(vk::ImageType::e2D)
        .setExtent(vk::Extent3D{extent.width, extent.height, 1})
        .setFormat(_format)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setMipLevels(1)
        .setArrayLayers(1)
        .setTiling(vk::ImageTiling::eOptimal);
    auto vk_ici = static_cast<VkImageCreateInfo>(ici);

    VmaAllocationCreateInfo i_alloc_info{};
    i_alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    i_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VK_CHECK(
        vmaCreateImage(
            _allocator,
            &vk_ici,
            &i_alloc_info,
            &_image.image,
            &_image.allocation,
            nullptr
        ),
        "Failed to allocate depth buffer image"
    );

    vk::ImageViewCreateInfo ivci{};
    ivci.setViewType(vk::ImageViewType::e2D)
        .setImage(vk::Image{_image.image})
        .setFormat(_format);
    ivci.subresourceRange.setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setAspectMask(vk::ImageAspectFlagBits::eDepth);

    _image_view = device.createImageViewUnique(ivci);
}

void DepthBuffer::destroy() {
    destroy_image(_image);
}

void DepthBuffer::destroy_image(AllocatedImage& buf) {
    if (buf.image || buf.allocation) {
        vmaDestroyImage(_allocator, buf.image, buf.allocation);
    }
}

}  // namespace hc
