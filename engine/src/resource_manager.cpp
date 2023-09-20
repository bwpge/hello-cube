#include "hvk/resource_manager.hpp"

namespace hvk {

ResourceManager::ResourceManager() {
    _materials[ResourceManager::Key{}] = std::make_unique<Material>(Material::none());
}

ResourceManager::Map<ResourceManager::Key, Unique<Shader>>& ResourceManager::get_shader_map(
    ShaderType type
) {
    switch (type) {
        case ShaderType::Vertex:
            return _vert_shaders;
        case ShaderType::Fragment:
            return _frag_shaders;
        case ShaderType::Geometry:
            return _geom_shaders;
        case ShaderType::Compute:
            return _comp_shaders;
    }

    panic("Unsupported shader type");
}

}  // namespace hvk
