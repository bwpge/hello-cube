#pragma once

#include <filesystem>
#include <unordered_map>
#include <string>
#include <string_view>

#include <spdlog/fmt/ostr.h>

#include "hvk/core.hpp"
#include "hvk/descriptor_utils.hpp"
#include "hvk/material.hpp"
#include "hvk/texture.hpp"
#include "hvk/shader.hpp"

namespace hvk {

struct TextureInfo {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::string name{};
    vk::Filter filter{vk::Filter::eLinear};
    vk::SamplerAddressMode mode{vk::SamplerAddressMode::eRepeat};

    // NOLINTEND(misc-non-private-member-variables-in-classes)

    bool operator==(const TextureInfo& other) const noexcept {
        return (
            name == other.name && filter == other.filter && mode == other.mode
        );
    }

    friend std::ostream& operator<<(std::ostream& os, const TextureInfo& self) {
        return os << "'" << self.name
                  << "' (filter=" << vk::to_string(self.filter)
                  << ", mode=" << vk::to_string(self.mode) << ")";
    }
};

}  // namespace hvk

template <>
struct fmt::formatter<hvk::TextureInfo> : fmt::ostream_formatter {};

template <>
struct std::hash<hvk::TextureInfo> {
    std::size_t operator()(const hvk::TextureInfo& info) const {
        std::size_t h1 = std::hash<decltype(info.name)>()(info.name);
        std::size_t h2 = std::hash<decltype(info.filter)>()(info.filter) << 1;
        std::size_t h3 = std::hash<decltype(info.mode)>()(info.mode) << 1;
        return ((h1 ^ h2) >> 1) ^ h3;
    }
};

namespace hvk {

class ResourceManager {
private:
    using Key = std::string;
    template <typename K, typename V>
    using Map = std::unordered_map<K, V>;

public:
    ResourceManager();

    static ResourceManager& get() {
        static ResourceManager instance{};
        return instance;
    }

    static const Shader& load_shader(
        const std::filesystem::path& path,
        ShaderType type,
        const Key& name = {}
    ) {
        auto key = name.empty() ? key_from_filename(path) : name;
        HVK_ASSERT(!key.empty(), "Shader resource name should not be empty");
        auto& map = get().get_shader_map(type);

        map[key] = std::make_unique<Shader>(Shader::load_spv(path));
        spdlog::trace("Created shader resource: '{}' (type={})", key, type);
        return *map[key];
    }

    static const Shader& shader(const std::string& name, ShaderType type) {
        return *get().get_shader_map(type).at(name);
    }

    static const Shader& vertex_shader(const std::string& name) {
        return shader(name, ShaderType::Vertex);
    }

    static const Shader& fragment_shader(const std::string& name) {
        return shader(name, ShaderType::Fragment);
    }

    static const ImageResource& load_image(
        const std::filesystem::path& path,
        UploadContext& ctx,
        const Key& name = {}
    ) {
        auto key = name.empty() ? key_from_filename(path) : name;
        HVK_ASSERT(!key.empty(), "Image resource name should not be empty");

        auto& map = get()._images;
        if (map.find(key) != map.end()) {
            spdlog::trace("Image resource '{}' already exists", key);
            return *map[key];
        }

        map[key] = std::make_unique<ImageResource>(path, ctx);
        spdlog::trace("Created image resource '{}'", key);
        return *map[key];
    }

    static Texture* texture(const TextureInfo& info) {
        auto& map = get()._textures;
        if (map.find(info) != map.end()) {
            return map[info].get();
        }

        spdlog::trace("Creating texture {}", info);
        const auto* resource_ptr = get()._images.at(info.name).get();
        HVK_ASSERT(
            resource_ptr,
            spdlog::fmt_lib::format("Image resource '{}' not found", info.name)
        );

        map[info] =
            std::make_unique<Texture>(*resource_ptr, info.filter, info.mode);
        return map[info].get();
    }

    static Texture* default_texture() {
        return ResourceManager::texture({});
    }

    static Material* make_material(
        const Key& name,
        const std::filesystem::path& base_dir,
        glm::vec3 ambient_base,
        const std::filesystem::path& ambient_tex,
        UploadContext& ctx
    ) {
        auto& map = get()._materials;
        if (map.find(name) != map.end()) {
            spdlog::trace("Material '{}' already exists", name);
            return map[name].get();
        }

        spdlog::trace("Creating material '{}'", name);
        Material material{};
        material.base_color_factor = glm::vec4{ambient_base, 1.0f};

        // check if texture is set
        if (!ambient_tex.empty()) {
            auto ambient = base_dir / ambient_tex;
            ResourceManager::load_image(ambient, ctx);
            TextureInfo tex_info{.name = ambient.stem().string()};
            material.base_color_texture = ResourceManager::texture(tex_info);
        }
        // if not lazy initialize empty texture and use that
        else {
            auto& images = get()._images;
            if (images.find({}) == images.end()) {
                images[{}] =
                    std::make_unique<ImageResource>(ImageResource::empty(ctx));
            }
            material.base_color_texture = ResourceManager::texture({});
        }

        map[name] = std::make_unique<Material>(material);
        return map[name].get();
    }

    static Material* material(const Key& name) {
        return get()._materials.at(name).get();
    }

    static Material* default_material() {
        return ResourceManager::material({});
    }

    static void prepare_materials(
        const vk::UniqueDescriptorPool& pool,
        const vk::UniqueDescriptorSetLayout& layout,
        const DescriptorSetBindingMap& binding_map
    ) {
        allocate_material_descriptors(pool, layout);
        update_material_descriptors(binding_map);
    }

private:
    static Key key_from_filename(const std::filesystem::path& path) {
        // attempt to remove all extensions
        auto key = path;
        auto stem = key.stem();
        while (!key.empty() && !stem.empty() && key != stem) {
            key = stem;
            stem = key.stem();
        }

        // use filename with extension as fallback
        if (key.empty()) {
            key = path.filename();
        }

        return key.string();
    }

    static void allocate_material_descriptors(
        const vk::UniqueDescriptorPool& pool,
        const vk::UniqueDescriptorSetLayout& layout
    ) {
        for (auto& [_, material] : get()._materials) {
            material->descriptor_set =
                VulkanContext::allocate_descriptor_set(pool, layout);
        }
    }

    static void update_material_descriptors(
        const DescriptorSetBindingMap& binding_map
    ) {
        DescriptorSetWriter writer{};
        for (auto& [_, material] : get()._materials) {
            std::vector<vk::DescriptorImageInfo> info{};
            if (material->base_color_texture) {
                info.push_back(material->base_color_texture->create_image_info()
                );
            } else {
                const auto* tex = ResourceManager::default_texture();
                info.push_back(tex->create_image_info());
            }
            writer.write_images(material->descriptor_set, binding_map, info);
        }
    }

    Map<Key, Unique<Shader>>& get_shader_map(ShaderType type);

    Map<Key, Unique<Shader>> _vert_shaders{};
    Map<Key, Unique<Shader>> _frag_shaders{};
    Map<Key, Unique<Shader>> _geom_shaders{};
    Map<Key, Unique<Shader>> _comp_shaders{};
    Map<Key, Unique<ImageResource>> _images{};
    Map<TextureInfo, Unique<Texture>> _textures{};
    Map<Key, Unique<Material>> _materials{};
};

}  // namespace hvk
