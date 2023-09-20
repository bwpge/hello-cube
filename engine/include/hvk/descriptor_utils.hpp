#pragma once

#include <map>

#include <vulkan/vulkan.hpp>

#include "hvk/core.hpp"

namespace hvk {

struct DescriptorDetails {
    vk::DescriptorType type{};
    vk::ShaderStageFlags stage_flags{};
    u32 count{1};
};

class DescriptorSetBindingMap {
    using Map = std::map<u32, DescriptorDetails>;

public:
    DescriptorSetBindingMap() = default;
    DescriptorSetBindingMap(std::initializer_list<DescriptorDetails> items);

    [[nodiscard]]
    Map::const_iterator begin() const noexcept;
    [[nodiscard]]
    Map::const_iterator end() const noexcept;
    [[nodiscard]]
    usize size() const noexcept;

    [[nodiscard]]
    const DescriptorDetails& at(u32 binding) const;
    vk::UniqueDescriptorSetLayout build_layout();

private:
    Map _details{};
};

class DescriptorSetLayoutBuilder {
public:
    DescriptorSetLayoutBuilder() = default;
    explicit DescriptorSetLayoutBuilder(const DescriptorSetBindingMap& map);

    DescriptorSetLayoutBuilder& add_binding(
        u32 binding,
        vk::DescriptorType type,
        vk::ShaderStageFlags stage_flags,
        u32 descriptor_count = 1
    );

    vk::UniqueDescriptorSetLayout build();

private:
    std::vector<vk::DescriptorSetLayoutBinding> _bindings{};
};

class DescriptorSetWriter {
public:
    DescriptorSetWriter& add_buffer_write(
        const vk::DescriptorSet& set,
        u32 binding,
        const DescriptorDetails& details,
        const vk::DescriptorBufferInfo& buffer_info
    );
    DescriptorSetWriter& add_image_write(
        const vk::DescriptorSet& set,
        u32 binding,
        const DescriptorDetails& details,
        const vk::DescriptorImageInfo& image_info
    );
    DescriptorSetWriter& write_buffers(
        const vk::DescriptorSet& set,
        const DescriptorSetBindingMap& binding_map,
        std::vector<vk::DescriptorBufferInfo> buffer_infos
    );
    DescriptorSetWriter& write_images(
        const vk::DescriptorSet& set,
        const DescriptorSetBindingMap& binding_map,
        std::vector<vk::DescriptorImageInfo> image_infos
    );

    void update();

private:
    std::vector<vk::WriteDescriptorSet> _writes{};
};

}  // namespace hvk
