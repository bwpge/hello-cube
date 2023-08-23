#include "shader.hpp"

namespace hc {

vk::UniqueShaderModule Shader::shader_module(const vk::Device& device) {
    auto info = vk::ShaderModuleCreateInfo{};
    info.setCodeSize(_buf.size());
    info.setPCode(reinterpret_cast<const u32*>(_buf.data()));
    return device.createShaderModuleUnique(info);
}

}  // namespace hc
