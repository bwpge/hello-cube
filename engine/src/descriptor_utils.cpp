#include "hvk/descriptor_utils.hpp"
#include "hvk/vk_context.hpp"

namespace hvk {

DescriptorSetBindingMap::DescriptorSetBindingMap(std::initializer_list<DescriptorDetails> items) {
    u32 idx{};
    for (const auto& item : items) {
        _details.insert_or_assign(idx++, item);
    }
}

DescriptorSetBindingMap::Map::const_iterator DescriptorSetBindingMap::begin() const noexcept {
    return _details.begin();
}

DescriptorSetBindingMap::Map::const_iterator DescriptorSetBindingMap::end() const noexcept {
    return _details.end();
}

usize DescriptorSetBindingMap::size() const noexcept {
    return _details.size();
}

const DescriptorDetails& DescriptorSetBindingMap::at(u32 binding) const {
    return _details.at(binding);
}

vk::UniqueDescriptorSetLayout DescriptorSetBindingMap::build_layout() {
    return DescriptorSetLayoutBuilder{*this}.build();
}

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(const DescriptorSetBindingMap& map) {
    for (const auto& [binding, item] : map) {
        this->add_binding(binding, item.type, item.stage_flags, item.count);
    }
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::add_binding(
    u32 binding,
    vk::DescriptorType type,
    vk::ShaderStageFlags stage_flags,
    u32 descriptor_count
) {
    vk::DescriptorSetLayoutBinding layout_binding{};
    layout_binding.setBinding(binding)
        .setDescriptorCount(descriptor_count)
        .setDescriptorType(type)
        .setStageFlags(stage_flags);
    _bindings.push_back(layout_binding);
    return *this;
}

vk::UniqueDescriptorSetLayout DescriptorSetLayoutBuilder::build() {
    auto layout = VulkanContext::device().createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo{{}, _bindings}
    );
    *this = {};
    return layout;
}

DescriptorSetWriter& DescriptorSetWriter::add_buffer_write(
    const vk::DescriptorSet& set,
    u32 binding,
    const DescriptorDetails& details,
    const vk::DescriptorBufferInfo& buffer_info
) {
    vk::WriteDescriptorSet write{};
    write.setDstSet(set)
        .setDstBinding(binding)
        .setDescriptorType(details.type)
        .setBufferInfo(buffer_info);

    _writes.push_back(write);
    return *this;
}

DescriptorSetWriter& DescriptorSetWriter::add_image_write(
    const vk::DescriptorSet& set,
    u32 binding,
    const DescriptorDetails& details,
    const vk::DescriptorImageInfo& image_info
) {
    vk::WriteDescriptorSet write{};
    write.setDstSet(set)
        .setDstBinding(binding)
        .setDescriptorType(details.type)
        .setImageInfo(image_info);

    _writes.push_back(write);
    return *this;
}

DescriptorSetWriter& DescriptorSetWriter::write_buffers(
    const vk::DescriptorSet& set,
    const DescriptorSetBindingMap& binding_map,
    vk::DescriptorBufferInfo buffer_infos
) {
    auto infos = std::vector<vk::DescriptorBufferInfo>{buffer_infos};
    return write_buffers(set, binding_map, infos);
}

DescriptorSetWriter& DescriptorSetWriter::write_buffers(
    const vk::DescriptorSet& set,
    const DescriptorSetBindingMap& binding_map,
    std::vector<vk::DescriptorBufferInfo> buffer_infos
) {
    HVK_ASSERT(
        binding_map.size() == buffer_infos.size(),
        "Number of buffers must be equal to number of mapped bindings"
    );
    for (u32 i = 0; i < buffer_infos.size(); i++) {
        add_buffer_write(set, i, binding_map.at(i), buffer_infos[i]);
    }

    this->update();
    return *this;
}

DescriptorSetWriter& DescriptorSetWriter::write_images(
    const vk::DescriptorSet& set,
    const DescriptorSetBindingMap& binding_map,
    vk::DescriptorImageInfo image_infos
) {
    auto infos = std::vector<vk::DescriptorImageInfo>{image_infos};
    return write_images(set, binding_map, infos);
}

DescriptorSetWriter& DescriptorSetWriter::write_images(
    const vk::DescriptorSet& set,
    const DescriptorSetBindingMap& binding_map,
    std::vector<vk::DescriptorImageInfo> image_infos
) {
    HVK_ASSERT(
        binding_map.size() == image_infos.size(),
        "Number of buffers must be equal to number of mapped bindings"
    );
    for (u32 i = 0; i < image_infos.size(); i++) {
        add_image_write(set, i, binding_map.at(i), image_infos[i]);
    }

    this->update();
    return *this;
}

void DescriptorSetWriter::update() {
    HVK_ASSERT(
        !_writes.empty(),
        "Cannot update descriptor sets without anything to write or copy"
    );
    VulkanContext::device().updateDescriptorSets(_writes, nullptr);
    *this = {};
}

}  // namespace hvk
