#pragma once

#include <filesystem>

#include <stb_image.h>
#include <vulkan/vulkan.hpp>

#include "hvk/core.hpp"
#include "hvk/vk_context.hpp"

namespace hvk {

class ImageResource {
public:
    friend class Texture;

    ImageResource() = default;
    ImageResource(const ImageResource&) = delete;
    ImageResource(ImageResource&& other) noexcept;
    ImageResource& operator=(const ImageResource&) = delete;
    ImageResource& operator=(ImageResource&& rhs) noexcept;
    ~ImageResource();

    static ImageResource empty(u8 r = 255, u8 g = 255, u8 b = 255, u8 a = 255) {
        spdlog::trace("Creating empty image resource: RGBA=[{}, {}, {}, {}]", r, g, b, a);

        ImageResource resource{};
        std::array<u8, 4> pixel = {r, g, b, a};
        resource._has_alpha = a != 255;

        resource.upload(pixel.data(), pixel.size(), 1, 1);
        return resource;
    }

    static ImageResource from_file(
        const std::filesystem::path& path,
        vk::Format format = vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal,
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
    ) {
        spdlog::trace("Loading image: '{}'", path.string());
        ImageResource resource{};

        i32 width{};
        i32 height{};
        i32 channels{};

        auto* pixels =
            stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        HVK_ASSERT(pixels, fmt::format("Failed to load image '{}'", path.string()));
        HVK_ASSERT(
            width && height && channels,
            fmt::format(
                "STB returned invalid image dimensions: width={}, height={}, channels={}",
                width,
                height,
                channels
            )
        );
        usize size = static_cast<usize>(width) * static_cast<usize>(height) * 4;

        // check if image has varying alpha
        for (usize i = 3; i < size; i += 4) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (pixels[i] != 255) {
                resource._has_alpha = true;
                break;
            }
        }

        resource.upload(pixels, size, width, height, format, layout, usage);
        stbi_image_free(pixels);
        return resource;
    }

    static ImageResource from_buffer(
        void* data,
        usize size,
        u32 width,
        u32 height,
        vk::Format format = vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal,
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
    ) {
        spdlog::trace("Loading image from buffer: {}", data);
        ImageResource resource{};
        resource.upload(data, size, width, height, format, layout, usage);
        return resource;
    }

    [[nodiscard]]
    vk::UniqueImageView create_image_view(
        vk::Format format = vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor
    ) const;

private:
    void upload(
        void* data,
        usize size,
        u32 width,
        u32 height,
        vk::Format format = vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal,
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
    );
    void destroy();

    AllocatedImage _image{};
    bool _has_alpha{false};
};

class TextureBase {
public:
    TextureBase() = default;
    TextureBase(const TextureBase&) = delete;
    TextureBase(TextureBase&&) = default;
    TextureBase& operator=(const TextureBase&) = delete;
    TextureBase& operator=(TextureBase&&) = default;
    virtual ~TextureBase() = default;

    [[nodiscard]]
    vk::DescriptorImageInfo descriptor_info(
        vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal
    ) const;
    [[nodiscard]]
    const vk::Sampler& sampler() const;
    [[nodiscard]]
    const vk::ImageView& image_view() const;

protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
    ImageResource _resource{};
    u32 _width{};
    u32 _height{};
    u32 _mip_levels{};
    u32 _layers{};
    vk::UniqueImageView _view{};
    vk::UniqueSampler _sampler{};
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes,misc-non-private-member-variables-in-classes)
};

class Texture2D : public TextureBase {
public:
    static Texture2D empty(u8 r = 255, u8 g = 255, u8 b = 255, u8 a = 255) {
        Texture2D tex{};
        tex._width = 1;
        tex._height = 1;
        tex._resource = ImageResource::empty(r, g, b, a);
        tex._view = tex._resource.create_image_view();

        vk::SamplerCreateInfo info{};
        info.setMagFilter(vk::Filter::eNearest)
            .setMinFilter(vk::Filter::eNearest)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat);
        tex._sampler = VulkanContext::device().createSamplerUnique(info);

        return tex;
    }

    static Texture2D from_buffer(
        void* data,
        usize size,
        u32 width,
        u32 height,
        vk::Format format = vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal,
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled,
        vk::Filter filter = vk::Filter::eLinear,
        vk::SamplerAddressMode addr_mode = vk::SamplerAddressMode::eRepeat
    ) {
        Texture2D tex{};
        tex._width = width;
        tex._height = height;
        tex._resource =
            ImageResource::from_buffer(data, size, width, height, format, layout, usage);
        tex._view = tex._resource.create_image_view(format);

        // TODO(bwpge): allow configuring each of these individually
        vk::SamplerCreateInfo info{};
        info.setMagFilter(filter)
            .setMinFilter(filter)
            .setAddressModeU(addr_mode)
            .setAddressModeV(addr_mode)
            .setAddressModeW(addr_mode);
        tex._sampler = VulkanContext::device().createSamplerUnique(info);

        return tex;
    }

    static Texture2D from_file(
        const std::filesystem::path& path,
        vk::Format format = vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout layout = vk::ImageLayout::eReadOnlyOptimal,
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled,
        vk::Filter filter = vk::Filter::eLinear,
        vk::SamplerAddressMode addr_mode = vk::SamplerAddressMode::eRepeat
    ) {
        Texture2D tex{};

        i32 w{};
        i32 h{};
        i32 n{};
        HVK_ASSERT(stbi_info(path.string().c_str(), &w, &h, &n) == 1, "STB failed to open image");
        HVK_ASSERT(w && h && n, "Failed to read image dimensions");
        tex._width = static_cast<u32>(w);
        tex._height = static_cast<u32>(h);

        tex._resource = ImageResource::from_file(path, format, layout, usage);
        tex._view = tex._resource.create_image_view(format);

        // TODO(bwpge): allow configuring each of these individually
        vk::SamplerCreateInfo info{};
        info.setMagFilter(filter)
            .setMinFilter(filter)
            .setAddressModeU(addr_mode)
            .setAddressModeV(addr_mode)
            .setAddressModeW(addr_mode);
        tex._sampler = VulkanContext::device().createSamplerUnique(info);

        return tex;
    }
};

class CubeMap : public TextureBase {
    // TODO(bwpge)
};

}  // namespace hvk
