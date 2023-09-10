#include "hvk/shader.hpp"

namespace hvk {

vk::UniqueShaderModule Shader::module() const {
    auto info = vk::ShaderModuleCreateInfo{};
    info.setPCode(reinterpret_cast<const u32*>(_buf.data()))
        .setCodeSize(_buf.size());
    return VulkanContext::device().createShaderModuleUnique(info);
}

void ShaderMap::load(
    Key key,
    ShaderType type,
    const std::filesystem::path& path
) {
    auto& map = get_by_name(type);
    map[key] = Shader::load_spv(path);
}

void ShaderMap::remove(Key key, ShaderType type) {
    auto& map = get_by_name(type);
    HVK_ASSERT(_vertex.contains(key), "");
    map.erase(key);
}

const Shader& ShaderMap::get(Key key, ShaderType type) const {
    const auto& map = get_by_name(type);
    return map.at(key);
}

vk::UniqueShaderModule ShaderMap::module(Key key, ShaderType type) const {
    return get(key, type).module();
}

const Shader& ShaderMap::vertex(Key key) const {
    return get(key, ShaderType::Vertex);
}

const Shader& ShaderMap::fragment(Key key) const {
    return get(key, ShaderType::Fragment);
}

const Shader& ShaderMap::geometry(Key key) const {
    return get(key, ShaderType::Geometry);
}

}  // namespace hvk
