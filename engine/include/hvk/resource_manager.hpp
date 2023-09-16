#pragma once

#include <filesystem>
#include <unordered_map>
#include <string>
#include <string_view>

#include "hvk/core.hpp"
#include "hvk/texture.hpp"
#include "hvk/shader.hpp"

namespace hvk {

struct TextureInfo {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::string name;
    vk::Filter filter;
    vk::SamplerAddressMode mode;

    // NOLINTEND(misc-non-private-member-variables-in-classes)

    bool operator==(const TextureInfo& other) const noexcept {
        return (
            name == other.name && filter == other.filter && mode == other.mode
        );
    }
};

}  // namespace hvk

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

        map[key] = std::make_unique<ImageResource>(path, ctx);
        spdlog::trace("Created image resource '{}'", key);
        return *map[key];
    }

    static const Texture& texture(const TextureInfo& info) {
        auto& map = get()._textures;
        if (map.find(info) != map.end()) {
            return *map[info];
        }

        const auto* resource_ptr = get()._images[info.name].get();
        HVK_ASSERT(
            resource_ptr,
            spdlog::fmt_lib::format("Image resource '{}' not found", info.name)
        );

        map[info] =
            std::make_unique<Texture>(*resource_ptr, info.filter, info.mode);
        return *map[info];
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

    Map<Key, Unique<Shader>>& get_shader_map(ShaderType type);

    Map<Key, Unique<Shader>> _vert_shaders{};
    Map<Key, Unique<Shader>> _frag_shaders{};
    Map<Key, Unique<Shader>> _geom_shaders{};
    Map<Key, Unique<Shader>> _comp_shaders{};
    Map<Key, Unique<ImageResource>> _images{};
    Map<TextureInfo, Unique<Texture>> _textures{};
};

}  // namespace hvk
