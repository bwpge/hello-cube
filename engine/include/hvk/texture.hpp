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
    ImageResource(const ImageResource&) = delete;
    ImageResource(ImageResource&& other) noexcept;
    ImageResource& operator=(const ImageResource&) = delete;
    ImageResource& operator=(ImageResource&& rhs) noexcept;
    ~ImageResource();

    static ImageResource load(
        const std::filesystem::path& path,
        UploadContext& ctx
    ) {
        spdlog::trace("Loading texture: {}", path.string());
        ImageResource texture{};

        i32 width{};
        i32 height{};
        i32 channels{};

        auto* pixels = stbi_load(
            path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha
        );
        HVK_ASSERT(pixels, "Failed to load texture");
        HVK_ASSERT(
            width && height && channels,
            spdlog::fmt_lib::format(
                "STB returned invalid image dimensions: width={}, height={}, "
                "channels={}",
                width,
                height,
                channels
            )
        );
        usize size = static_cast<usize>(width) * static_cast<usize>(height) * 4;

        // check if texture has varying alpha
        for (usize i = 3; i < size; i += 4) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (pixels[i] != 255) {
                texture._has_alpha = true;
                break;
            }
        }

        auto& allocator = VulkanContext::allocator();

        // copy image data to buffer
        auto staging_buf = allocator.create_staging_buffer(size);
        allocator.copy_mapped(staging_buf, pixels, size);

        // clean up image, since we have all data in the buffer
        stbi_image_free(pixels);

        vk::Format format = vk::Format::eR8G8B8A8Srgb;
        vk::Extent3D extent{
            static_cast<u32>(width),
            static_cast<u32>(height),
            1,
        };
        vk::ImageCreateInfo create_info{};
        create_info.setImageType(vk::ImageType::e2D)
            .setExtent(extent)
            .setFormat(format)
            .setUsage(
                vk::ImageUsageFlagBits::eSampled |
                vk::ImageUsageFlagBits::eTransferDst
            )
            .setSamples(vk::SampleCountFlagBits::e1)
            .setMipLevels(1)
            .setArrayLayers(1)
            .setTiling(vk::ImageTiling::eOptimal);

        auto image = allocator.create_image(
            create_info,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        );

        ctx.oneshot(
            VulkanContext::graphics_queue(),
            [=](const vk::UniqueCommandBuffer& cmd) {
                // transition image to receive data
                vk::ImageSubresourceRange range{
                    vk::ImageAspectFlagBits::eColor,
                    0,
                    1,
                    0,
                    1,
                };
                vk::ImageMemoryBarrier barrier_xfer{};
                barrier_xfer.setImage(image.image)
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
                    barrier_xfer
                );

                // copy image data from staging buffer
                vk::BufferImageCopy copy_region{};
                copy_region.setBufferOffset(0)
                    .setBufferRowLength(0)
                    .setBufferImageHeight(0)
                    .setImageExtent(extent);
                copy_region.imageSubresource
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setMipLevel(0)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1);
                cmd->copyBufferToImage(
                    staging_buf.buffer,
                    image.image,
                    vk::ImageLayout::eTransferDstOptimal,
                    copy_region
                );

                // transition to shader readable image
                vk::ImageMemoryBarrier barrier_readable{};
                barrier_readable.setImage(image.image)
                    .setSubresourceRange(range)
                    .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                    .setNewLayout(vk::ImageLayout::eReadOnlyOptimal)
                    .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
                cmd->pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eFragmentShader,
                    {},
                    nullptr,
                    nullptr,
                    barrier_readable
                );
            }
        );

        allocator.destroy(staging_buf);
        std::swap(texture._image, image);

        return texture;
    }

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
