#define STB_IMAGE_IMPLEMENTATION
#include "hvk/texture.hpp"
#include "hvk/vk_context.hpp"

namespace hvk {

vk::DescriptorImageInfo TextureBase::descriptor_info(vk::ImageLayout layout) const {
    return {
        _sampler.get(),
        _view.get(),
        layout,
    };
}

const vk::Sampler& TextureBase::sampler() const {
    return _sampler.get();
}

const vk::ImageView& TextureBase::image_view() const {
    return _view.get();
}

ImageResource::ImageResource(ImageResource&& other) noexcept {
    if (this == &other) {
        return;
    }
    std::swap(_image, other._image);
    std::swap(_has_alpha, other._has_alpha);
}

ImageResource& ImageResource::operator=(ImageResource&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    destroy();
    std::swap(_image, rhs._image);
    std::swap(_has_alpha, rhs._has_alpha);

    return *this;
}

ImageResource::~ImageResource() {
    destroy();
}

vk::UniqueImageView ImageResource::create_image_view(
    vk::Format format,
    vk::ImageAspectFlags aspect_mask  //
) const {
    vk::ImageViewCreateInfo create_info{};
    create_info.setImage(_image.image).setViewType(vk::ImageViewType::e2D).setFormat(format);

    // TODO(bwpge): fix level count for mip chain
    create_info.subresourceRange.setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setAspectMask(aspect_mask);

    return VulkanContext::device().createImageViewUnique(create_info);
}

void ImageResource::upload(
    void* data,
    usize size,
    u32 width,
    u32 height,
    vk::Format format,
    vk::ImageLayout layout,
    vk::ImageUsageFlags usage
)  //
{
    auto& allocator = VulkanContext::allocator();

    // copy image data to buffer
    auto staging_buf = allocator.create_staging_buffer(size);
    allocator.copy_mapped(staging_buf, data, size);

    vk::Extent3D extent{
        static_cast<u32>(width),
        static_cast<u32>(height),
        1,
    };

    // TODO(bwpge): fix level count for mip chain
    vk::ImageCreateInfo create_info{};
    create_info.setImageType(vk::ImageType::e2D)
        .setExtent(extent)
        .setFormat(format)
        .setUsage(usage | vk::ImageUsageFlagBits::eTransferDst)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setMipLevels(1)
        .setArrayLayers(1)
        .setTiling(vk::ImageTiling::eOptimal);

    auto image = allocator.create_image(
        create_info,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    );

    auto cmd = VulkanContext::oneshot();
    vk::ImageSubresourceRange range{};
    // TODO(bwpge): fix level count for mip chain
    range.setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(1).setLevelCount(1);

    // transition image to receive data
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.setImage(image.image)
            .setSubresourceRange(range)
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcAccessMask({})
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        cmd->pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr,
            nullptr,
            barrier
        );
    }

    // copy image data from staging buffer
    vk::BufferImageCopy copy_region{};
    copy_region.setBufferOffset(0).setBufferRowLength(0).setBufferImageHeight(0).setImageExtent(
        extent
    );
    copy_region.imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setMipLevel(0)
        .setBaseArrayLayer(0)
        .setLayerCount(1);
    cmd->copyBufferToImage(
        staging_buf.buffer,
        image.image,
        vk::ImageLayout::eTransferDstOptimal,
        copy_region
    );

    // transition to final layout
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.setImage(image.image)
            .setSubresourceRange(range)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(layout)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        cmd->pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            nullptr,
            nullptr,
            barrier
        );
    }

    VulkanContext::flush_command_buffer(cmd, VulkanContext::transfer_queue());
    allocator.destroy(staging_buf);
    std::swap(_image, image);
}

void ImageResource::destroy() {
    VulkanContext::allocator().destroy(_image);
}

}  // namespace hvk
