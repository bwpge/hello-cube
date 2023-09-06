#include "shader.hpp"

namespace hvk {

vk::UniqueShaderModule Shader::module(const vk::UniqueDevice& device) const {
    return module(device.get());
}

vk::UniqueShaderModule Shader::module(const vk::Device& device) const {
    auto info = vk::ShaderModuleCreateInfo{};
    info.setCodeSize(_buf.size());
    info.setPCode(reinterpret_cast<const u32*>(_buf.data()));
    return device.createShaderModuleUnique(info);
}

void ShaderMap::load(
    Key key,
    ShaderType type,
    const std::filesystem::path& path
) {
    auto& map = get_by_name(type);
    map[key] = Shader::load_spv(path);
}

void ShaderMap::remove(const Key key, ShaderType type) {
    auto& map = get_by_name(type);
    HVK_ASSERT(_vertex.contains(key), "");
    map.erase(key);
}

const Shader& ShaderMap::get(const Key key, ShaderType type) {
    const auto& map = get_by_name(type);
    return map.at(key);
}

}  // namespace hvk
